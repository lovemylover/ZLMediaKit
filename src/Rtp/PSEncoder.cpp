/*
 * Copyright (c) 2016-present The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/ZLMediaKit/ZLMediaKit).
 *
 * Use of this source code is governed by MIT-like license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#if defined(ENABLE_RTPPROXY)

#include "PSEncoder.h"
#include "Common/config.h"
#include "Extension/CommonRtp.h"
#include "Rtsp/RtspMuxer.h"
#include "Util/onceToken.h"

using namespace toolkit;

namespace mediakit{

// 池化对象引用计数归零自动回收复用，消除每次 PS/TS 输出块的堆分配
class PooledFrameFromPtr : public FrameFromPtr {
public:
    using Ptr = std::shared_ptr<PooledFrameFromPtr>;

    static Ptr create(CodecId codec_id, char *ptr, size_t size, uint64_t dts, uint64_t pts, size_t prefix_size, bool is_key) {
        static ResourcePool<PooledFrameFromPtr> packet_pool;
        static onceToken token([]() {
            packet_pool.setSize(64);
        });
        auto ret = packet_pool.obtain2();
        ret->_ptr = ptr;
        ret->_size = size;
        ret->_dts = dts;
        ret->_pts = pts;
        ret->_prefix_size = prefix_size;
        ret->_is_key = is_key;
        ret->_codec_id = codec_id;
        return ret;
    }

private:
    friend class ResourcePool_l<PooledFrameFromPtr>;
    PooledFrameFromPtr() = default;
};

PSEncoderImp::PSEncoderImp(uint32_t ssrc, uint8_t payload_type, bool ps_or_ts) : MpegMuxer(ps_or_ts) {
    GET_CONFIG(uint32_t, s_video_mtu, Rtp::kVideoMtuSize);
    _rtp_encoder = std::make_shared<CommonRtpEncoder>();
    auto video_mtu = s_video_mtu;
    if (!ps_or_ts) {
        // 确保ts rtp负载部分长度是188的倍数  [AUTO-TRANSLATED:ad7aa6c0]
        // Ensure the ts rtp payload length is a multiple of 188
        video_mtu = RtpPacket::kRtpHeaderSize + (s_video_mtu - (s_video_mtu % 188));
        if (video_mtu > s_video_mtu) {
            video_mtu -= 188;
        }
    }
    _rtp_encoder->setRtpInfo(ssrc, video_mtu, 90000, payload_type);
    auto ring = std::make_shared<RtpRing::RingType>();
    ring->setDelegate(std::make_shared<RingDelegateHelper>([this](RtpPacket::Ptr rtp, bool is_key) { onRTP(std::move(rtp), is_key); }));
    _rtp_encoder->setRtpRing(std::move(ring));
    InfoL << this << " " << ssrc;
}

PSEncoderImp::~PSEncoderImp() {
    InfoL << this;
}

void PSEncoderImp::onWrite(std::shared_ptr<Buffer> buffer, uint64_t stamp, bool key_pos) {
    if (!buffer) {
        return;
    }
    _rtp_encoder->inputFrame(PooledFrameFromPtr::create(CodecH264/*只用于识别为视频*/, buffer->data(), buffer->size(), stamp, stamp, 0, key_pos));
}

}//namespace mediakit

#endif//defined(ENABLE_RTPPROXY)
