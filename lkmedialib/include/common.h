#ifndef COMMON_H
#define COMMON_H

extern "C"{
    #include "libavutil/pixfmt.h"
    #include "libavutil/rational.h"
    #include "libavcodec/avcodec.h"
    #include "libavutil/imgutils.h"
}

#include "queue"
#include "mutex"
#include "condition_variable"
#include "functional"

class video_information{
public:
    video_information() = default;
    ~video_information() = default;

    void init(AVPixelFormat fmt, int w, int h, long long bit_rate, AVRational video_rate, AVRational time_base);


    AVPixelFormat fmt() const;
    void setFmt(const AVPixelFormat &fmt);

    int getWidht() const;
    void setWidht(int value);

    int getHeight() const;
    void setHeight(int value);

    long long getBit_rate() const;
    void setBit_rate(long long bit_rate);

    AVRational getVideo_rate() const;
    void setVideo_rate(const AVRational &video_rate);

    AVRational getTime_base() const;
    void setTime_base(const AVRational &time_base);

private:
    AVPixelFormat fmt_;
    int widht_, height_;
    long long bit_rate_;
    AVRational video_rate_;
    AVRational time_base_;
};

/**
 * @brief The grab_option class
 *
 * 用于为recordr指定抓取选项
 *
 */

class grab_option {
public:
    grab_option() = default;

    grab_option(bool has_window,
                  bool has_camera,
                  bool has_audio,
                  AVRational frame_rate,
                  AVPixelFormat c = AV_PIX_FMT_RGB24,
                  AVPixelFormat w = AV_PIX_FMT_RGB24);

    ~grab_option() = default;

    void init(bool has_window,
              bool has_camera,
              bool has_audio,
              AVRational frame_rate,
              AVPixelFormat c = AV_PIX_FMT_RGB24,
              AVPixelFormat w = AV_PIX_FMT_RGB24);

    bool has_window() const;
    void setHas_window(bool has_window);

    bool has_camera() const;
    void setHas_camera(bool has_camera);

    bool has_audio() const;
    void setHas_audio(bool has_audio);

    AVRational getFrame_rate() const;
    void setFrame_rate(const AVRational &value);

    AVPixelFormat getDest_camera_fmt() const;
    void setDest_camera_fmt(const AVPixelFormat &dest_camera_fmt);

    AVPixelFormat getDest_window_fmt() const;
    void setDest_window_fmt(const AVPixelFormat &dest_window_fmt);

private:
    bool has_window_;
    bool has_camera_;
    bool has_audio_;
    AVRational frame_rate_;
    AVPixelFormat dest_camera_fmt_;
    AVPixelFormat dest_window_fmt_;
};

class mux_option{
public:
    mux_option() = default;
    ~mux_option() = default;

    bool has_audio() const;
    void setHas_audio(bool has_audio);

    bool mux_frame() const;
    void setMux_frame(bool mux_frame);

    std::string save_file_path() const;
    void setSave_file_path(const std::string &save_file_path);

    const grab_option& grab_option() const;
    void setGrab_option(const class grab_option &grab_option);

    std::function<void (AVFrame *)> video_callback() const;
    void setVideo_callback(const std::function<void (AVFrame *)> &video_callback);

    std::function<void (AVFrame *)> camera_callback() const;
    void setCamera_callback(const std::function<void (AVFrame *)> &camera_callback);

    std::function<void ()> stop_callback() const;
    void setStop_callback(const std::function<void ()> &stop_callback);

private:
    bool has_audio_;
    bool mux_frame_; // 标记是否将流写入到save_file_path当中
    std::string save_file_path_;
    class grab_option grab_option_;
    std::function<void(AVFrame*)> video_callback_;
    std::function<void(AVFrame*)> camera_callback_;
    std::function<void()> stop_callback_;
};

class framequeue{
public:
    framequeue() = default;
    ~framequeue() = default;

    void put(AVFrame*);

    AVFrame* get();

    bool empty();

    long long getFrame_count() const;
    void setFrame_count(long long frame_count);

    long long getFrame_size() const;
    void setFrame_size(long long frame_size);

private:
    long long frame_count_;
    long long frame_size_;

    std::queue<AVFrame*> frame_queue_;
    std::mutex queue_mtx_;
    std::condition_variable queue_cond_not_full_;
    std::condition_variable queue_cond_not_empty_;

    static constexpr long long max_queue_size = 200;

};

void show_codec_context_information(AVCodec* codec, AVCodecContext* ctx, int idx);

void show_frame_information(AVFrame*  frame);

#endif // COMMON_H
