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

class lkgraber{
public:
    lkgraber() = default;
    ~lkgraber() = default;

    /**
     * @brief init 初始化录制器
     * @param ro 录制选项
     * @param video_callback 桌面帧回调，用于自定义使用桌面帧
     * @param camera_callback 摄像头帧回调，用于自定义如何使用视频帧
     * @param stop_callback_ 暂停回调，用于定义在暂停录制之后功能
     */

    virtual void init(grab_option ro, \
                      std::function<void(AVFrame* frame)> video_callback = nullptr, \
                      std::function<void(AVFrame* frame)> camera_callback = nullptr, \
                      std::function<void(void)> stop_callback_ = nullptr) = 0;

    /**
     * @brief record 开始录制
     *
     * 默认只输出camera摄像头流，视频流需求需要提供ro选项
     *
     */

    virtual void grab() = 0;

    /**
     * @brief stop 录制暂停
     *
     * 可提供stop_callback用于定义暂停之后的行为
     *
     */

    virtual void stop() = 0;

    virtual int getCamera_width() = 0;
    virtual void setCamera_width(int value) = 0;

    virtual int getCamera_height() = 0;
    virtual void setCamera_height(int value) = 0;

};

/**
 * @brief The recoder class 实现录制器，用于实现截取摄像头屏幕
 *        以及截取整个windows的屏幕，内部拥有lkmuxer复用器，用于
 *        实现将编码好的音视频packet写入媒体文件当中。
 *        (首先实现视频的流的编码）
 *        输入 -> 录制器 -> 视频帧输出
 */

class graber: public lkgraber
{
public:
    graber();
    void init(grab_option ro, \
              std::function<void(AVFrame* frame)> video_callback = nullptr, \
              std::function<void(AVFrame* frame)> camera_callback = nullptr,
              std::function<void(void)> stop_callback = nullptr) override;
    void grab() override;
    void stop() override;


    int getCamera_width() override;
    void setCamera_width(int value) override;

    int getCamera_height() override;
    void setCamera_height(int value) override;

private:
    grab_option grab_option_; // 指定录制的模式（包含窗口，包含摄像头，包含音频, 录制速率）

    bool record_flag_;

    /**
     * @brief camera_callback_
     *
     * 回调函数，用于处理recoder抓取的摄像头帧
     *
     */

    std::function<void(AVFrame* frame)> camera_callback_;

    /**
     * @brief video_callback_
     *
     * 回调函数，用于处理recoder抓取的视频帧
     *
     * @param frame 指针指向解析出来的视频帧，需要使用av_image_ref去引用一下，
     *        否则退出回调之后，frame指向的内存空间会被释放
     */
    std::function<void(AVFrame* frame)> video_callback_; // 回调，用于处理录制的视频帧

    std::function<void(void)> stop_callback_;

    bool thread_exited_flag_;
    std::mutex thread_exited_flag_mtx;
    std::condition_variable thread_exited_flag_cond_;

    int camera_width_;
    int camera_height_;
    std::mutex height_width_mtx_;
    std::condition_variable height_width_cond_;
    bool can_get_height_width_;

    static constexpr int max_record_camera_width =  1920;
    static constexpr int max_record_camera_height =  1080;
};

#endif // RECODER_H
