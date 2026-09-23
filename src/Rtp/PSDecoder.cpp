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

#include "PSDecoder.h"
#include "mpeg-ps.h"

using namespace toolkit;

namespace mediakit{

PSDecoder::PSDecoder() {
    rebuildDemuxer();
}

PSDecoder::~PSDecoder() {
    ps_demuxer_destroy(static_cast<struct ps_demuxer_t *>(_ps_demuxer));
}

ssize_t PSDecoder::input(const uint8_t *data, size_t bytes) {
    try {
        ps_demuxer_input(static_cast<struct ps_demuxer_t *>(_ps_demuxer), data, bytes);
    } catch (AssertFailedException &ex) {
        InfoL << "解析 ps 异常: bytes=" << bytes
              << ", exception=" << ex.what()
              << ", hex=" << hexdump(data, MIN(bytes, 32));
        // 销毁重建，恢复解析能力
        rebuildDemuxer();
    }
    return bytes;
}

void PSDecoder::clearInputCache() {
    // 重建 demuxer 以清空内部残留缓冲
    rebuildDemuxer();
}

void PSDecoder::rebuildDemuxer() {
    if (_ps_demuxer) {
        ps_demuxer_destroy(static_cast<struct ps_demuxer_t *>(_ps_demuxer));
        _ps_demuxer = nullptr;
    }

    _ps_demuxer = ps_demuxer_create([](void* param,
                                       int stream,
                                       int codecid,
                                       int flags,
                                       int64_t pts,
                                       int64_t dts,
                                       const void* data,
                                       size_t bytes){
        PSDecoder *thiz = static_cast<PSDecoder *>(param);
        if(thiz->_on_decode){
            thiz->_on_decode(stream, codecid, flags, pts, dts, data, bytes);
        }
        return 0;
    }, this);

    ps_demuxer_notify_t notify = {
            [](void *param, int stream, int codecid, const void *extra, int bytes, int finish) {
                PSDecoder *thiz = static_cast<PSDecoder *>(param);
                if (thiz->_on_stream) {
                    thiz->_on_stream(stream, codecid, extra, bytes, finish);
                }
            }
    };
    ps_demuxer_set_notify(static_cast<struct ps_demuxer_t *>(_ps_demuxer), &notify, this);
}

}//namespace mediakit
#endif//#if defined(ENABLE_RTPPROXY)
