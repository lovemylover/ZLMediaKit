/*
 * Copyright (c) 2016-present The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/ZLMediaKit/ZLMediaKit).
 *
 * Use of this source code is governed by MIT-like license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#ifndef ZLMEDIAKIT_H264_H
#define ZLMEDIAKIT_H264_H

#include "Extension/Frame.h"
#include "Extension/Track.h"
#include "Util/ResourcePool.h"

#define H264_TYPE(v) ((uint8_t)(v) & 0x1F)

namespace mediakit{

void splitH264(const char *ptr, size_t len, size_t prefix, const std::function<void(const char *, size_t, size_t)> &cb);
size_t prefixSize(const char *ptr, size_t len);

template<typename Parent>
class H264FrameHelper : public Parent{
public:
    friend class FrameImp;
    friend class toolkit::ResourcePool_l<H264FrameHelper>;
    using Ptr = std::shared_ptr<H264FrameHelper>;

    // 从对象池创建，减少每帧堆分配；仅适用于 Parent = FrameFromPtr 的实例化(H264FrameNoCacheAble)
    // Create from object pool to reduce per-frame heap allocation; only for the Parent = FrameFromPtr instantiation (H264FrameNoCacheAble)
    static Ptr create() {
        static toolkit::ResourcePool<H264FrameHelper> packet_pool;
        static toolkit::onceToken token([]() {
            packet_pool.setSize(1024);
        });
        auto ret = packet_pool.obtain2();
        // 全量重置，避免对象复用时残留上一帧的 _ptr/_size 等成员
        // Reset all members to avoid stale data from the previous frame when the object is reused
        ret->init(nullptr, 0, 0, 0, 0, false);
        // _index 由下游 setIndex 赋值，需复位为未设置状态，与原构造语义保持一致
        // _index is assigned by setIndex downstream; reset it to the unset state to match the original constructor semantics
        ret->setIndex(-1);
        return ret;
    }

    // 初始化对象池中取出的对象，替代构造函数传参
    // Initialize the object obtained from the pool, replacing constructor arguments
    void init(char *ptr, size_t size, uint64_t dts, uint64_t pts = 0, size_t prefix_size = 0, bool is_key = false) {
        this->_ptr = ptr;
        this->_size = size;
        this->_dts = dts;
        this->_pts = pts;
        this->_prefix_size = prefix_size;
        this->_is_key = is_key;
        this->_codec_id = CodecH264;
    }

    enum {
        NAL_IDR = 5,
        NAL_SEI = 6,
        NAL_SPS = 7,
        NAL_PPS = 8,
        NAL_AUD = 9,
        NAL_B_P = 1,
    };

    template<typename ...ARGS>
    H264FrameHelper(ARGS &&...args): Parent(std::forward<ARGS>(args)...) {
        this->_codec_id = CodecH264;
    }

    bool keyFrame() const override {
        auto nal_ptr = (uint8_t *) this->data() + this->prefixSize();
        return H264_TYPE(*nal_ptr) == NAL_IDR && decodeAble();
    }

    bool configFrame() const override {
        auto nal_ptr = (uint8_t *) this->data() + this->prefixSize();
        switch (H264_TYPE(*nal_ptr)) {
            case NAL_SPS:
            case NAL_PPS: return true;
            default: return false;
        }
    }

    bool dropAble() const override {
        auto nal_ptr = (uint8_t *) this->data() + this->prefixSize();
        switch (H264_TYPE(*nal_ptr)) {
            case NAL_SEI:
            case NAL_AUD: return true;
            default: return false;
        }
    }

    bool decodeAble() const override {
        auto nal_ptr = (uint8_t *) this->data() + this->prefixSize();
        auto type = H264_TYPE(*nal_ptr);
        // 多slice情况下, first_mb_in_slice 表示其为一帧的开始  [AUTO-TRANSLATED:80e88e88]
        // // In the case of multiple slices, first_mb_in_slice indicates the start of a frame
        return type >= NAL_B_P && type <= NAL_IDR && (nal_ptr[1] & 0x80);
    }
};

/**
 * 264帧类
 * 264 frame class
 
 * [AUTO-TRANSLATED:342ccb1e]
 */
using H264Frame = H264FrameHelper<FrameImp>;

/**
 * 防止内存拷贝的H264类
 * 用户可以通过该类型快速把一个指针无拷贝的包装成Frame类
 * H264 class that prevents memory copying
 * Users can quickly wrap a pointer into a Frame class without copying using this type
 
 * [AUTO-TRANSLATED:ff9be1c8]
 */
using H264FrameNoCacheAble = H264FrameHelper<FrameFromPtr>;

/**
 * 264视频通道
 * 264 video channel
 
 * [AUTO-TRANSLATED:6936e76d]
 */
class H264Track : public VideoTrack {
public:
    using Ptr = std::shared_ptr<H264Track>;

    /**
     * 不指定sps pps构造h264类型的媒体
     * 在随后的inputFrame中获取sps pps
     * Construct a media of h264 type without specifying sps pps
     * Get sps pps in the subsequent inputFrame
     
     * [AUTO-TRANSLATED:84d01c7f]
     */
    H264Track() = default;

    /**
     * 构造h264类型的媒体
     * @param sps sps帧数据
     * @param pps pps帧数据
     * @param sps_prefix_len 264头长度，可以为3个或4个字节，一般为0x00 00 00 01
     * @param pps_prefix_len 264头长度，可以为3个或4个字节，一般为0x00 00 00 01
     * Construct a media of h264 type
     * @param sps sps frame data
     * @param pps pps frame data
     * @param sps_prefix_len 264 header length, can be 3 or 4 bytes, generally 0x00 00 00 01
     * @param pps_prefix_len 264 header length, can be 3 or 4 bytes, generally 0x00 00 00 01
     
     
     * [AUTO-TRANSLATED:702c1433]
     */
    H264Track(const std::string &sps, const std::string &pps, int sps_prefix_len = 4, int pps_prefix_len = 4);

    bool ready() const override;
    CodecId getCodecId() const override;
    int getVideoHeight() const override;
    int getVideoWidth() const override;
    float getVideoFps() const override;
    bool inputFrame(const Frame::Ptr &frame) override;
    toolkit::Buffer::Ptr getExtraData() const override;
    void setExtraData(const uint8_t *data, size_t size) override;
    bool update() override;
    std::vector<Frame::Ptr> getConfigFrames() const override;

private:
    Sdp::Ptr getSdp(uint8_t payload_type) const override;
    Track::Ptr clone() const override;
    bool inputFrame_l(const Frame::Ptr &frame);
    void insertConfigFrame(const Frame::Ptr &frame);

    bool latestIsConfigFrame();

private:
    bool _latest_is_pps = false;
    bool _latest_is_sps = false;
    int _width = 0;
    int _height = 0;
    float _fps = 0;
    std::string _sps;
    std::string _pps;
};

template <typename FrameType>
Frame::Ptr createConfigFrame(const std::string &data, uint64_t dts, int index) {
    auto frame = FrameImp::create<FrameType>();
    frame->_prefix_size = 4;
    frame->_buffer.assign("\x00\x00\x00\x01", 4);
    frame->_buffer.append(data);
    frame->_dts = dts;
    frame->setIndex(index);
    return frame;
}

}//namespace mediakit

#endif //ZLMEDIAKIT_H264_H
