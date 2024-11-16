#ifndef FRAMEMUXER_H
#define FRAMEMUXER_H

#include "common.h"
#include "graber.h"

class lkrecoder{
public:
    lkrecoder() = default;
    virtual ~lkrecoder() = default;

    virtual void init(std::string save_file_path,
                      bool has_camera,
                      bool has_window,
                      bool has_audio,
                      bool mux_frame,
                      AVRational frame_rate,
                      std::function<void (AVFrame *)> video_callback,
                      std::function<void (AVFrame *)> camera_callback,
                      std::function<void ()> stop_callback) = 0;

    virtual void record() = 0;

    virtual void set_video_callback(std::function<void(AVFrame*)> = nullptr) = 0;

    virtual void set_camera_callback(std::function<void(AVFrame*)> = nullptr) = 0;

    virtual void set_stop_callback(std::function<void()> = nullptr) = 0;

};

class recoder: public lkrecoder
{
public:
    recoder() = default;
    ~recoder() override = default;

    void init(std::string save_file_path,
              bool has_camera,
              bool has_window,
              bool has_audio,
              bool mux_frame,
              AVRational frame_rate,
              std::function<void (AVFrame *)> video_callback,
              std::function<void (AVFrame *)> camera_callback,
              std::function<void ()> stop_callback) override;

    void record() override;

    void set_video_callback(std::function<void(AVFrame*)> = nullptr) override;

    void set_camera_callback(std::function<void(AVFrame*)> = nullptr) override;

    void set_stop_callback(std::function<void()> = nullptr) override;

private:

    void consumer();

    mux_option mux_option_;

    framequeue video_frame_queue_;
    framequeue audio_frame_queue_;

    graber graber_;
};

#endif // FRAMEMUXER_H
