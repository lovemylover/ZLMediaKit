/*
 * Copyright (c) 2016-present The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/ZLMediaKit/ZLMediaKit).
 *
 * Use of this source code is governed by MIT-like license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#ifndef ZLMEDIAKIT_PSDECODER_H
#define ZLMEDIAKIT_PSDECODER_H

#if defined(ENABLE_RTPPROXY)
#include <stdint.h>
#include "Decoder.h"

namespace mediakit{

// ps解析器  [AUTO-TRANSLATED:f156a1f1]
// ps parser
class PSDecoder : public Decoder {
public:
    PSDecoder();
    ~PSDecoder();

    ssize_t input(const uint8_t* data, size_t bytes) override;
    void clearInputCache() override;

private:
    // 销毁旧实例并重建 ps_demuxer，恢复回调注册
    void rebuildDemuxer();

    void *_ps_demuxer = nullptr;
};

}//namespace mediakit
#endif//defined(ENABLE_RTPPROXY)
#endif //ZLMEDIAKIT_PSDECODER_H
