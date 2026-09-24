/*
 * Copyright (c) 2016-present The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/ZLMediaKit/ZLMediaKit).
 *
 * Use of this source code is governed by MIT-like license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#include "RtmpMuxer.h"
#include "Extension/Factory.h"

namespace mediakit {

RtmpMuxer::RtmpMuxer(const TitleMeta::Ptr &title) {
    if (!title) {
        _metadata = std::make_shared<TitleMeta>()->getMetadata();
    } else {
        _metadata = title->getMetadata();
    }
    _rtmp_ring = std::make_shared<RtmpRing::RingType>();
}

RtmpCodec::Ptr &RtmpMuxer::getEncoder(int index) {
    // 双槽memo快路径：整数比较，命中则跳过哈希查找
    // Two-slot memo fast path: integer comparison, skip the hash lookup on hit
    for (size_t i = 0; i < 2; ++i) {
        if (_encoder_memo_key[i] == index && _encoder_memo[i]) {
            return *_encoder_memo[i];
        }
    }
    // miss: 回退operator[]（保持插入默认值语义），回填一个槽位
    // miss: fall back to operator[] (maintain insert-default-value semantics), backfill a slot
    auto &ref = _encoders[index];
    size_t slot_idx = _encoder_memo_next++ & 1;
    _encoder_memo[slot_idx] = &ref;
    _encoder_memo_key[slot_idx] = index;
    return ref;
}

bool RtmpMuxer::addTrack(const Track::Ptr &track) {
    if (_track_existed[track->getTrackType()]) {
        // rtmp不支持多个同类型track  [AUTO-TRANSLATED:c69a7864]
        // rtmp does not support multiple tracks of the same type
        WarnL << "Already add a track kind of: " << track->getTrackTypeStr() << ", ignore track: " << track->getCodecName();
        return false;
    }

    auto &encoder = _encoders[track->getIndex()];
    // 新增元素保守清空memo（unordered_map为节点式容器，rehash不会使元素指针失效，此处仅为与先例一致并防御容器变更）
    // Conservatively clear the memo when adding an element (unordered_map is a node-based container, rehash does not invalidate element pointers; this is only to stay consistent with the precedent and defend against container changes)
    _encoder_memo[0] = nullptr;
    _encoder_memo[1] = nullptr;
    _encoder_memo_key[0] = -1;
    _encoder_memo_key[1] = -1;
    CHECK(!encoder);
    encoder = Factory::getRtmpEncoderByTrack(track);
    if (!encoder) {
        return false;
    }

    // 标记已经存在该类型track  [AUTO-TRANSLATED:ed79ebb5]
    // Mark that a track of this type already exists
    _track_existed[track->getTrackType()] = true;

    // 设置rtmp输出环形缓存  [AUTO-TRANSLATED:d65af70c]
    // Set the rtmp output circular buffer
    encoder->setRtmpRing(_rtmp_ring);

    // 添加metadata  [AUTO-TRANSLATED:eaf2f5ae]
    // Add metadata
    Metadata::addTrack(_metadata, track);
    return true;
}

bool RtmpMuxer::inputFrame(const Frame::Ptr &frame) {
    auto &encoder = getEncoder(frame->getIndex());
    return encoder ? encoder->inputFrame(frame) : false;
}

void RtmpMuxer::flush() {
    for (auto &pr : _encoders) {
        if (pr.second) {
            pr.second->flush();
        }
    }
}

void RtmpMuxer::makeConfigPacket() {
    for (auto &pr : _encoders) {
        if (pr.second) {
            pr.second->makeConfigPacket();
        }
    }
}

const AMFValue &RtmpMuxer::getMetadata() const {
    return _metadata;
}

RtmpRing::RingType::Ptr RtmpMuxer::getRtmpRing() const {
    return _rtmp_ring;
}

void RtmpMuxer::resetTracks() {
    _metadata.clear();
    _encoders.clear();
    // 清空后节点全部销毁，memo指针全部悬空
    // After clearing, all nodes are destroyed and all memo pointers dangle
    _encoder_memo[0] = nullptr;
    _encoder_memo[1] = nullptr;
    _encoder_memo_key[0] = -1;
    _encoder_memo_key[1] = -1;
    CLEAR_ARR(_track_existed);
}

} /* namespace mediakit */