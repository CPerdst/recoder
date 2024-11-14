#ifndef COMMON_H
#define COMMON_H

extern "C"{
    #include "libavutil/pixfmt.h"
    #include "libavutil/rational.h"
    #include "libavcodec/avcodec.h"
    #include "libavutil/imgutils.h"
}

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

class record_option {
public:
    record_option();

    record_option(bool has_window,
                  bool has_camera,
                  bool has_audio,
                  AVRational frame_rate,
                  AVPixelFormat c = AV_PIX_FMT_RGB24,
                  AVPixelFormat w = AV_PIX_FMT_RGB24);

    ~record_option();

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



class common
{
public:
    common();
};

#endif // COMMON_H
