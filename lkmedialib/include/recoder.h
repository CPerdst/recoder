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
                      std::function<void (AVFrame *)> video_callback = nullptr,
                      std::function<void (AVFrame *)> camera_callback = nullptr,
                      std::function<void ()> stop_callback = nullptr) = 0;

    virtual void record() = 0;

    virtual void stop() = 0;

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
              std::function<void (AVFrame *)> video_callback = nullptr,
              std::function<void (AVFrame *)> camera_callback = nullptr,
              std::function<void ()> stop_callback = nullptr) override;

    void record() override;

    void stop() override;

    void set_video_callback(std::function<void(AVFrame*)> = nullptr) override;

    void set_camera_callback(std::function<void(AVFrame*)> = nullptr) override;

    void set_stop_callback(std::function<void()> = nullptr) override;

private:

    void consumer();

    mux_option mux_option_;

    framequeue video_frame_queue_;
    framequeue audio_frame_queue_;

    graber graber_;

    bool recording_;
    bool consumer_exit_flag_;
    std::mutex consumer_exit_mtx_;
    std::condition_variable consumer_exit_cond_;
};

#endif // FRAMEMUXER_H
