/*
 * Copyright (c) 2021 The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/ZLMediaKit/ZLMediaKit).
 *
 * Use of this source code is governed by MIT-like license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#include "TwccContext.h"
#include <algorithm>
#include "Rtcp/RtcpFCI.h"

namespace mediakit {

// 在升序数组中二分查找twcc_ext_seq，返回其迭代器(未找到则为插入点)
static inline std::vector<std::pair<uint32_t, uint64_t>>::iterator
findTwccSeq(std::vector<std::pair<uint32_t, uint64_t>> &status, uint32_t twcc_ext_seq) {
    return std::lower_bound(status.begin(), status.end(), twcc_ext_seq,
                            [](const std::pair<uint32_t, uint64_t> &pr, uint32_t seq) { return pr.first < seq; });
}

enum class ExtSeqStatus : int {
    normal = 0,
    looped,
    jumped,
};

void TwccContext::onRtp(uint32_t ssrc, uint16_t twcc_ext_seq, uint64_t stamp_ms) {
    switch ((ExtSeqStatus) checkSeqStatus(twcc_ext_seq)) {
        case ExtSeqStatus::jumped: /*seq异常,过滤掉*/ return;
        case ExtSeqStatus::looped: /*回环，触发发送twcc rtcp*/ onSendTwcc(ssrc); break;
        case ExtSeqStatus::normal: break;
        default: /*不可达*/assert(0); break;
    }

    // 窗口不会超过kMaxSeqSize，一次性预分配容量，此后clear()保留容量，稳态下无任何堆分配/释放  [AUTO-TRANSLATED:530d1e35]
    // The window never exceeds kMaxSeqSize, so pre-allocate capacity once. Afterwards clear() keeps the
    // capacity, so there is no heap allocation/free in steady state
    if (_rtp_recv_status.capacity() == 0) {
        _rtp_recv_status.reserve(2 * kMaxSeqSize);
    }

    // 保持升序插入(窗口极小，二分+insert开销可忽略)  [AUTO-TRANSLATED:530d1e35]
    // Keep the array sorted on insert (the window is tiny, so binary search + insert is negligible)
    auto it = findTwccSeq(_rtp_recv_status, twcc_ext_seq);
    if (it != _rtp_recv_status.end() && it->first == twcc_ext_seq) {
        // 重复的twcc ext seq，保持首次记录的时间戳(first wins)  [AUTO-TRANSLATED:530d1e35]
        // Duplicate twcc ext seq, keep the timestamp recorded the first time (first wins)
        WarnL << "recv same twcc ext seq:" << twcc_ext_seq;
        return;
    }
    it = _rtp_recv_status.insert(it, std::make_pair((uint32_t) twcc_ext_seq, stamp_ms));

    // 与map版语义一致：取刚插入元素的时间戳  [AUTO-TRANSLATED:530d1e35]
    // Same semantics as the map version: take the timestamp of the newly inserted element
    _max_stamp = it->second;
    if (!_min_stamp) {
        _min_stamp = _max_stamp;
    }

    if (needSendTwcc()) {
        // 其他匹配条件立即发送twcc  [AUTO-TRANSLATED:959d22b6]
        // Send twcc immediately if other matching conditions are met
        onSendTwcc(ssrc);
    }
}

bool TwccContext::needSendTwcc() const {
    if (_rtp_recv_status.empty()) {
        return false;
    }
    return (_rtp_recv_status.size() >= kMaxSeqSize) || (_max_stamp - _min_stamp >= kMaxTimeDelta);
}

int TwccContext::checkSeqStatus(uint16_t twcc_ext_seq) const {
    if (_rtp_recv_status.empty()) {
        return (int) ExtSeqStatus::normal;
    }
    auto max = _rtp_recv_status.back().first;
    auto delta = (int32_t) twcc_ext_seq - (int32_t) max;
    if (delta > 0 && delta < 0xFFFF / 2) {
        // 正常增长  [AUTO-TRANSLATED:7699c37d]
        // Normal growth
        return (int) ExtSeqStatus::normal;
    }
    if (delta < -0xFF00) {
        // 回环  [AUTO-TRANSLATED:82956e03]
        // Loop
        TraceL << "rtp twcc ext seq looped:" << max << " -> " << twcc_ext_seq;
        return (int) ExtSeqStatus::looped;
    }
    if (delta > 0xFF00) {
        // 回环后收到前面大的乱序的包，无法处理，丢弃  [AUTO-TRANSLATED:512cc269]
        // Discard packets that are out of order and large after looping back, as they cannot be processed
        TraceL << "rtp twcc ext seq jumped after looped:" << max << " -> " << twcc_ext_seq;
        return (int) ExtSeqStatus::jumped;
    }
    auto min = _rtp_recv_status.front().first;
    if (min <= twcc_ext_seq || twcc_ext_seq <= max) {
        // 正常回退  [AUTO-TRANSLATED:c8c6803f]
        // Normal rollback
        return (int) ExtSeqStatus::normal;
    }
    // seq莫名的大幅增加或减少，无法处理，丢弃  [AUTO-TRANSLATED:44dbbd28]
    // Discard packets with a large increase or decrease in seq, as they cannot be processed
    TraceL << "rtp twcc ext seq jumped:" << max << " -> " << twcc_ext_seq;
    return (int) ExtSeqStatus::jumped;
}

void TwccContext::onSendTwcc(uint32_t ssrc) {
    auto max = _rtp_recv_status.back().first;
    auto min = _rtp_recv_status.front().first;
    // 参考时间戳的最小单位是64ms  [AUTO-TRANSLATED:2e701a8c]
    // The minimum unit of the reference timestamp is 64ms
    auto ref_time = _rtp_recv_status.front().second >> 6;
    // 还原基准时间戳  [AUTO-TRANSLATED:bab53195]
    // Restore the baseline timestamp
    auto last_time = ref_time << 6;
    FCI_TWCC::TwccPacketStatus status;
    for (auto seq = min; seq <= max; ++seq) {
        int16_t delta = 0;
        SymbolStatus symbol = SymbolStatus::not_received;
        // 数组有序，二分查找seq  [AUTO-TRANSLATED:46a0e186]
        // The array is sorted, so binary search for the seq
        auto it = findTwccSeq(_rtp_recv_status, seq);
        if (it != _rtp_recv_status.end() && it->first == seq) {
            // recv delta,单位为250us,1ms等于4x250us  [AUTO-TRANSLATED:46a0e186]
            // recv delta, unit is 250us, 1ms equals 4x250us
            delta = (int16_t) (4 * ((int64_t) it->second - (int64_t) last_time));
            if (delta < 0 || delta > 0xFF) {
                symbol = SymbolStatus::large_delta;
            } else {
                symbol = SymbolStatus::small_delta;
            }
            last_time = it->second;
        }
        status.emplace(seq, std::make_pair(symbol, delta));
    }
    auto fci = FCI_TWCC::create(ref_time, _twcc_pkt_count++, status);
    if (_cb) {
        _cb(ssrc, std::move(fci));
    }
    clearStatus();
}

void TwccContext::clearStatus() {
    _rtp_recv_status.clear();
    _min_stamp = 0;
}

void TwccContext::setOnSendTwccCB(TwccContext::onSendTwccCB cb) {
    _cb = std::move(cb);
}

}// namespace mediakit