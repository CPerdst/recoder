#include "common.h"
#include "eventCapturer.h"

void video_information::init(AVPixelFormat fmt, int w, int h, long long bit_rate, AVRational video_rate, AVRational time_base)
{
    fmt_ = fmt;
    widht_ = w;
    height_ = h;
    bit_rate_ = bit_rate;
    video_rate_ = video_rate;
    time_base_ = time_base;
}

AVPixelFormat video_information::fmt() const
{
    return fmt_;
}

void video_information::setFmt(const AVPixelFormat &fmt)
{
    fmt_ = fmt;
}

int video_information::getWidht() const
{
    return widht_;
}

void video_information::setWidht(int value)
{
    widht_ = value;
}

int video_information::getHeight() const
{
    return height_;
}

void video_information::setHeight(int value)
{
    height_ = value;
}

long long video_information::getBit_rate() const
{
    return bit_rate_;
}

void video_information::setBit_rate(long long bit_rate)
{
    bit_rate_ = bit_rate;
}

AVRational video_information::getVideo_rate() const
{
    return video_rate_;
}

void video_information::setVideo_rate(const AVRational &video_rate)
{
    video_rate_ = video_rate;
}

AVRational video_information::getTime_base() const
{
    return time_base_;
}

void video_information::setTime_base(const AVRational &time_base)
{
    time_base_ = time_base;
}

grab_option::grab_option(bool has_window,
                             bool has_camera,
                             bool has_audio,
                             AVRational frame_rate,
                             AVPixelFormat c,
                             AVPixelFormat w)
{
    has_window_ = has_window;
    has_camera_ = has_camera;
    has_audio_ = has_audio;
    frame_rate_ = frame_rate;
    dest_camera_fmt_ = c;
    dest_window_fmt_ = w;
}

void grab_option::init(bool has_window,
                         bool has_camera,
                         bool has_audio,
                         AVRational frame_rate,
                         AVPixelFormat c,
                         AVPixelFormat w)
{
    has_window_ = has_window;
    has_camera_ = has_camera;
    has_audio_ = has_audio;
    frame_rate_ = frame_rate;
    dest_camera_fmt_ = c;
    dest_window_fmt_ = w;
}

bool grab_option::has_window() const
{
    return has_window_;
}

void grab_option::setHas_window(bool has_window)
{
    has_window_ = has_window;
}

bool grab_option::has_camera() const
{
    return has_camera_;
}

void grab_option::setHas_camera(bool has_camera)
{
    has_camera_ = has_camera;
}

bool grab_option::has_audio() const
{
    return has_audio_;
}

void grab_option::setHas_audio(bool has_audio)
{
    has_audio_ = has_audio;
}

AVRational grab_option::getFrame_rate() const
{
    return frame_rate_;
}

void grab_option::setFrame_rate(const AVRational &value)
{
    frame_rate_ = value;
}

AVPixelFormat grab_option::getDest_camera_fmt() const
{
    return dest_camera_fmt_;
}

void grab_option::setDest_camera_fmt(const AVPixelFormat &dest_camera_fmt)
{
    dest_camera_fmt_ = dest_camera_fmt;
}

AVPixelFormat grab_option::getDest_window_fmt() const
{
    return dest_window_fmt_;
}

void grab_option::setDest_window_fmt(const AVPixelFormat &dest_window_fmt)
{
    dest_window_fmt_ = dest_window_fmt;
}

void show_codec_context_information(AVCodec* codec, AVCodecContext* ctx, int idx){
    RootDebug() << "----------CODEC CONTEXT INFO----------";
    RootDebug() << codec->long_name;
    if(ctx->codec_type == AVMEDIA_TYPE_AUDIO){
        RootDebug() << "Stream:        " << idx;
        RootDebug() << "Sample Format: " << av_get_sample_fmt_name(ctx->sample_fmt);
        RootDebug() << "Sample Size:   " << av_get_bytes_per_sample(ctx->sample_fmt);
        RootDebug() << "Channels:      " << ctx->channels;
        RootDebug() << "Float Output:  " << (av_sample_fmt_is_planar(ctx->sample_fmt) ? "yes" : "no");
        RootDebug() << "Sample Rate:   " << ctx->sample_rate;
        RootDebug() << "Audio TimeBase: " << av_q2d(ctx->time_base);
    }else if(ctx->codec_type == AVMEDIA_TYPE_VIDEO){
        RootDebug() << "Stream:        " << idx;
        RootDebug() << "Video Format: " << av_get_pix_fmt_name(ctx->pix_fmt);
        RootDebug() << "Video Height: " << ctx->height;
        RootDebug() << "Video Width: " << ctx->width;
        RootDebug() << "Video Rate: " << av_q2d(ctx->framerate);
        RootDebug() << "Video TimeBase: " << av_q2d(ctx->time_base);
    }
}

void show_frame_information(AVFrame*  frame){
    RootDebug() << "----------FRAME INFO----------";
    RootDebug() << "frame format: " << av_get_sample_fmt_name((enum AVSampleFormat)frame->format);
    RootDebug() << "frame rate: " << frame->sample_rate;
    RootDebug() << "frame channel layout: " << frame->channel_layout;
    RootDebug() << "frame channels: " << frame->channels;
}

void framequeue::put(AVFrame *frame)
{
    std::unique_lock<std::mutex> lock(queue_mtx_);
    if(frame_count_ == max_queue_size){
        queue_cond_not_full_.wait(lock, [this](){return frame_count_ < max_queue_size;});
    }
    frame_queue_.push(frame);
    frame_count_++;
    queue_cond_not_empty_.notify_one();
}

AVFrame *framequeue::get()
{
    std::unique_lock<std::mutex> lock(queue_mtx_);
    if(frame_count_ == 0){
        queue_cond_not_empty_.wait(lock, [this](){return frame_count_ > 0;});
    }
//    qDebug() << frame_count_;
    auto ele = frame_queue_.front();
    frame_queue_.pop();
    frame_count_--;
    queue_cond_not_full_.notify_one();
    return ele;
}

bool framequeue::empty()
{
    std::unique_lock<std::mutex> lock(queue_mtx_);
    return frame_count_ == 0;
}

long long framequeue::getFrame_count() const
{
    return frame_count_;
}

void framequeue::setFrame_count(long long frame_count)
{
    frame_count_ = frame_count;
}

long long framequeue::getFrame_size() const
{
    return frame_size_;
}

void framequeue::setFrame_size(long long frame_size)
{
    frame_size_ = frame_size;
}

bool mux_option::has_audio() const
{
    return has_audio_;
}

void mux_option::setHas_audio(bool has_audio)
{
    has_audio_ = has_audio;
}

bool mux_option::mux_frame() const
{
    return mux_frame_;
}

void mux_option::setMux_frame(bool mux_frame)
{
    mux_frame_ = mux_frame;
}

std::string mux_option::save_file_path() const
{
    return save_file_path_;
}

void mux_option::setSave_file_path(const std::string &save_file_path)
{
    save_file_path_ = save_file_path;
}

const grab_option& mux_option::grab_option() const
{
    return grab_option_;
}

void mux_option::setGrab_option(const class grab_option &grab_option)
{
    grab_option_ = grab_option;
}

std::function<void (AVFrame *)> mux_option::video_callback() const
{
    return video_callback_;
}

void mux_option::setVideo_callback(const std::function<void (AVFrame *)> &video_callback)
{
    video_callback_ = video_callback;
}

std::function<void (AVFrame *)> mux_option::camera_callback() const
{
    return camera_callback_;
}

void mux_option::setCamera_callback(const std::function<void (AVFrame *)> &camera_callback)
{
    camera_callback_ = camera_callback;
}

std::function<void ()> mux_option::stop_callback() const
{
    return stop_callback_;
}

void mux_option::setStop_callback(const std::function<void ()> &stop_callback)
{
    stop_callback_ = stop_callback;
}
