#include "common.h"

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


common::common()
{

}


record_option::record_option()
{

}

record_option::record_option(bool has_window,
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

record_option::~record_option()
{

}

void record_option::init(bool has_window,
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

bool record_option::has_window() const
{
    return has_window_;
}

void record_option::setHas_window(bool has_window)
{
    has_window_ = has_window;
}

bool record_option::has_camera() const
{
    return has_camera_;
}

void record_option::setHas_camera(bool has_camera)
{
    has_camera_ = has_camera;
}

bool record_option::has_audio() const
{
    return has_audio_;
}

void record_option::setHas_audio(bool has_audio)
{
    has_audio_ = has_audio;
}

AVRational record_option::getFrame_rate() const
{
    return frame_rate_;
}

void record_option::setFrame_rate(const AVRational &value)
{
    frame_rate_ = value;
}

AVPixelFormat record_option::getDest_camera_fmt() const
{
    return dest_camera_fmt_;
}

void record_option::setDest_camera_fmt(const AVPixelFormat &dest_camera_fmt)
{
    dest_camera_fmt_ = dest_camera_fmt;
}

AVPixelFormat record_option::getDest_window_fmt() const
{
    return dest_window_fmt_;
}

void record_option::setDest_window_fmt(const AVPixelFormat &dest_window_fmt)
{
    dest_window_fmt_ = dest_window_fmt;
}
