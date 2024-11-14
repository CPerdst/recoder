#ifndef RECODER_H
#define RECODER_H

extern "C"{
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libavutil/pixfmt.h"
    #include "libavutil/imgutils.h"
    #include "libswscale/swscale.h"
}

#include "memory"
#include "common.h"
#include "mutex"
#include "functional"
#include "condition_variable"

/**
 * @brief The formartranslater class 设置目的视频帧格式，
 *        通过提供原始格式数据，转换成目标格式数据
 */
class formartranslater{
public:
    formartranslater();
    ~formartranslater();
};

/**
 * @brief The lkrecoder class 录制器接口
 */

class lkrecoder{
public:
    lkrecoder() = default;
    ~lkrecoder() = default;

    virtual void init(record_option ro, \
                      std::function<void(AVFrame* frame)> video_callback = nullptr, \
                      std::function<void(AVFrame* frame)> camera_callback = nullptr, \
                      std::function<void(void)> stop_callback_ = nullptr) = 0;
    virtual void record() = 0;
    virtual void stop() = 0;
};

/**
 * @brief The recoder class 实现录制器，用于实现截取摄像头屏幕
 *        以及截取整个windows的屏幕，内部拥有lkmuxer复用器，用于
 *        实现将编码好的音视频packet写入媒体文件当中。
 *        (首先实现视频的流的编码）
 */

class recoder: public lkrecoder
{
public:
    recoder();
    void init(record_option ro, \
              std::function<void(AVFrame* frame)> video_callback = nullptr, \
              std::function<void(AVFrame* frame)> camera_callback = nullptr,
              std::function<void(void)> stop_callback = nullptr) override;
    void record() override;
    void stop() override;


private:
    record_option record_option_; // 指定录制的模式（包含窗口，包含摄像头，包含音频, 录制速率）

    bool record_flag_;

    std::function<void(AVFrame* frame)> camera_callback_;

    /**
     * @brief video_callback_
     *
     * @param frame 指针指向解析出来的视频帧，需要使用av_image_ref去引用一下，
     *        否则退出回调之后，frame指向的内存空间会被释放
     */
    std::function<void(AVFrame* frame)> video_callback_; // 回调，用于处理录制的视频帧

    std::function<void(void)> stop_callback_;

    bool thread_exited_flag_;
    std::mutex thread_exited_flag_mtx;
    std::condition_variable thread_exited_flag_cond_;

    static constexpr int max_record_camera_width =  1920;
    static constexpr int max_record_camera_height =  1080;
};

#endif // RECODER_H
