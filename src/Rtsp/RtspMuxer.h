/*
 * Copyright (c) 2016-present The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/ZLMediaKit/ZLMediaKit).
 *
 * Use of this source code is governed by MIT-like license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#ifndef ZLMEDIAKIT_RTSPMUXER_H
#define ZLMEDIAKIT_RTSPMUXER_H

#include "Extension/Frame.h"
#include "Common/MediaSink.h"
#include "Common/Stamp.h"
#include "RtpCodec.h"

namespace mediakit{

class RingDelegateHelper : public toolkit::RingDelegate<RtpPacket::Ptr> {
public:
    using onRtp = std::function<void(RtpPacket::Ptr in, bool is_key)> ;

    RingDelegateHelper(onRtp on_rtp) {
        _on_rtp = std::move(on_rtp);
    }

    void onWrite(RtpPacket::Ptr in, bool is_key) override {
        _on_rtp(std::move(in), is_key);
    }

private:
    onRtp _on_rtp;
};

/**
* rtsp生成器
 * RTSP generator
 
 * [AUTO-TRANSLATED:2a72d801]
*/
class RtspMuxer : public MediaSinkInterface {
public:
    using Ptr = std::shared_ptr<RtspMuxer>;

    /**
     * 构造函数
     * Constructor
     
     * [AUTO-TRANSLATED:41469869]
     */
    RtspMuxer(const TitleSdp::Ptr &title = nullptr);

    /**
     * 获取完整的SDP字符串
     * @return SDP字符串
     * Get the complete SDP string
     * @return SDP string
     
     * [AUTO-TRANSLATED:f5d1b0a6]
     */
    std::string getSdp() ;

    /**
     * 获取rtp环形缓存
     * @return
     * Get the RTP ring buffer
     * @return
     
     * [AUTO-TRANSLATED:644e8634]
     */
    RtpRing::RingType::Ptr getRtpRing() const;

    /**
     * 添加ready状态的track
     * Add a ready state track
     
     * [AUTO-TRANSLATED:2d8138b3]
     */
    bool addTrack(const Track::Ptr & track) override;

    /**
     * 写入帧数据
     * @param frame 帧
     * Write frame data
     * @param frame Frame
     
     * [AUTO-TRANSLATED:b7c92013]
     */
    bool inputFrame(const Frame::Ptr &frame) override;

    /**
     * 刷新输出所有frame缓存
     * Flush all frame buffers
     
     * [AUTO-TRANSLATED:adaea568]
     */
    void flush() override;

    /**
     * 重置所有track
     * Reset all tracks
     
     
     * [AUTO-TRANSLATED:f203fa3e]
     */
    void resetTracks() override ;

private:
    void onRtp(RtpPacket::Ptr in, bool is_key);
    void trySyncTrack();

private:
    bool _live = true;
    bool _track_existed[2] = { false, false };

    uint8_t _index {0};
    uint64_t _ntp_stamp_start;
    std::string _sdp;

    struct TrackInfo {
        Stamp stamp;
        uint32_t rtp_stamp { 0 };
        uint64_t ntp_stamp { 0 };
        RtpCodec::Ptr encoder;
        // sample_rate 在流生命周期内固定（addTrack 时由 SDP 确定），缓存以消除 onRtp 点播路径每包整数除法
        uint32_t sample_rate { 0 };
        // 1000.0 / sample_rate；标准采样率下双精度乘法截断与整数除法 stamp*1000/rate 一致（已验证）
        double ms_per_tick { 0 };
    };

    // 获取 track 对应的 TrackInfo，优先命中 memo，miss 时回退哈希查找并回填
    // 注意保持 operator[] 语义：不存在时插入默认构造的 TrackInfo
    TrackInfo &getTrackInfo(int index);

    std::unordered_map<int, TrackInfo> _tracks;
    // track_index -> TrackInfo 双槽 memo，消除每包哈希查找
    TrackInfo *_track_memo[2] = { nullptr, nullptr };
    int _track_memo_key[2] = { -1, -1 };
    size_t _track_memo_next = 0;
    RtpRing::RingType::Ptr _rtpRing;
    RtpRing::RingType::Ptr _rtpInterceptor;
};


} /* namespace mediakit */

#endif //ZLMEDIAKIT_RTSPMUXER_H
