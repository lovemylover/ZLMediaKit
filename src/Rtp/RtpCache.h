/*
 * Copyright (c) 2016-present The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/ZLMediaKit/ZLMediaKit).
 *
 * Use of this source code is governed by MIT-like license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#ifndef ZLMEDIAKIT_RTPCACHE_H
#define ZLMEDIAKIT_RTPCACHE_H

#if defined(ENABLE_RTPPROXY)

#include <vector>
#include "PSEncoder.h"
#include "RawEncoder.h"
#include "Common/PacketCache.h"

namespace mediakit{

// rtp转发路径每包都会追加到合并写缓存，用vector替代默认的List(std::list)，
// 消除每个rtp包一次list节点malloc/释放；仅此处特化，不影响RTMP/TS/FMP4等其他协议的PacketCache
// Every RTP packet is appended to the merge-write cache, so a vector is used instead of the default
// List (std::list) to avoid one list-node malloc/free per RTP packet. Only this specialization is
// changed, so PacketCache users such as RTMP/TS/FMP4 are unaffected
class RtpCache : protected PacketCache<toolkit::Buffer, FlushPolicy, std::vector<std::shared_ptr<toolkit::Buffer> > > {
public:
    using onFlushed = std::function<void(std::shared_ptr<std::vector<toolkit::Buffer::Ptr> >)>;
    RtpCache(onFlushed cb);

protected:
    /**
     * 输入rtp(目的是为了合并写)
     * @param buffer rtp数据
     * Input rtp (for merging)
     * @param buffer rtp data
     
     
     * [AUTO-TRANSLATED:de9469b5]
     */
    void input(uint64_t stamp, toolkit::Buffer::Ptr buffer,bool is_key = false);

protected:
    void onFlush(std::shared_ptr<std::vector<toolkit::Buffer::Ptr> > rtp_list, bool) override;

private:
    onFlushed _cb;
};

class RtpCachePS : public RtpCache, public PSEncoderImp {
public:
    RtpCachePS(onFlushed cb, uint32_t ssrc, uint8_t payload_type = 96, bool ps_or_ts = true) :
        RtpCache(std::move(cb)), PSEncoderImp(ssrc, ps_or_ts ? payload_type : static_cast<int>(Rtsp::PT_MP2T), ps_or_ts) {};

    void flush() override;

protected:
    void onRTP(toolkit::Buffer::Ptr rtp, bool is_key = false) override;
};

class RtpCacheRaw : public RtpCache, public RawEncoderImp {
public:
    RtpCacheRaw(onFlushed cb, uint32_t ssrc, uint8_t payload_type = 96, bool send_audio = true) : RtpCache(std::move(cb)), RawEncoderImp(ssrc, payload_type, send_audio) {};
    void flush() override;

protected:
    void onRTP(toolkit::Buffer::Ptr rtp, bool is_key = false) override;
};

} //namespace mediakit

#endif//ENABLE_RTPPROXY
#endif //ZLMEDIAKIT_RTPCACHE_H
