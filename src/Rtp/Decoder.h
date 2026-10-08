/*
 * Copyright (c) 2016-present The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/ZLMediaKit/ZLMediaKit).
 *
 * Use of this source code is governed by MIT-like license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#ifndef ZLMEDIAKIT_DECODER_H
#define ZLMEDIAKIT_DECODER_H

#include <stdint.h>
#include <memory>
#include <functional>
#include <array>
#include "Common/MediaSink.h"

namespace mediakit {

class Decoder {
public:
    using Ptr = std::shared_ptr<Decoder>;
    using onDecode = std::function<void(int stream, int codecid, int flags, int64_t pts, int64_t dts, const void *data, size_t bytes)>;
    using onStream = std::function<void(int stream, int codecid, const void *extra, size_t bytes, int finish)>;

    virtual ssize_t input(const uint8_t *data, size_t bytes) = 0;
    // 清理未完成的输入缓存，但不 flush 已解析的帧。
    // Clear incomplete input cache without flushing parsed frames.
    virtual void clearInputCache() {}
    void setOnDecode(onDecode cb);
    void setOnStream(onStream cb);

protected:
    Decoder() = default;
    virtual ~Decoder() = default;

protected:
    onDecode _on_decode;
    onStream _on_stream;
};

class DecoderImp {
public:
    typedef enum { decoder_ts = 0, decoder_ps } Type;

    using Ptr = std::shared_ptr<DecoderImp>;

    static Ptr createDecoder(Type type, MediaSinkInterface *sink);
    ssize_t input(const uint8_t *data, size_t bytes);
    void clearInputCache();
    void flush();

protected:
    void onTrack(int index, const Track::Ptr &track);
    void onFrame(int index, const Frame::Ptr &frame);

private:
    DecoderImp(const Decoder::Ptr &decoder, MediaSinkInterface *sink);
    void onDecode(int stream, int codecid, int flags, int64_t pts, int64_t dts, const void *data, size_t bytes);
    void onStream(int stream, int codecid, const void *extra, size_t bytes, int finish);

private:
    bool _finished = false;
    bool _have_video = false;
    bool _last_is_keyframe = false;
    Decoder::Ptr _decoder;
    MediaSinkInterface *_sink;
    class FrameMergerImp : public FrameMerger {
    public:
        FrameMergerImp() : FrameMerger(FrameMerger::none) {}
    };
    FrameMergerImp *_video_merge = nullptr;
    std::unordered_map<int, std::pair<Track::Ptr, FrameMergerImp> > _tracks;

    // 2 槽 stream memo：稳态 stream 集合 <= 2（video/audio），命中跳过 hash 查找。
    // std::unordered_map 节点地址稳定，插入/rehash 不 invalidate，可安全缓存指针。
    std::pair<Track::Ptr, FrameMergerImp> *getTrackRef(int stream) {
        for (auto &slot : _track_memo) {
            if (slot.first == stream && slot.second) return slot.second;
        }
        auto &ref = _tracks[stream];
        auto &slot = _track_memo[_track_memo_next++ & 1];
        slot.first = stream;
        slot.second = &ref;
        return &ref;
    }
    std::array<std::pair<int, std::pair<Track::Ptr, FrameMergerImp>*>, 2> _track_memo{ {{-1, nullptr}, {-1, nullptr}} };
    size_t _track_memo_next = 0;
};

}//namespace mediakit
#endif //ZLMEDIAKIT_DECODER_H
