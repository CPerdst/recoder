#include "recoder.h"

std::function<void()> video_callback_ = nullptr;

std::function<void()> audio_callback_ = nullptr;

void recoder::init(std::string save_file_path,
                   bool has_camera,
                   bool has_window,
                   bool has_audio,
                   bool mux_frame,
                   AVRational frame_rate,
                   std::function<void (AVFrame *)> video_callback,
                   std::function<void (AVFrame *)> camera_callback,
                   std::function<void ()> stop_callback)
{
    mux_option_.setSave_file_path(save_file_path);
    grab_option g;
    g.setHas_window(has_window);
    g.setHas_camera(has_camera);
    g.setFrame_rate(frame_rate);
    mux_option_.setGrab_option(g);
    mux_option_.setHas_audio(has_audio);
    mux_option_.setMux_frame(mux_frame);

    graber_.init(mux_option_.grab_option(),
                 video_callback,
                 camera_callback,
                 stop_callback);
}

void recoder::set_video_callback(std::function<void (AVFrame *)> c)
{
    if(c) mux_option_.setVideo_callback(c);
}

void recoder::set_camera_callback(std::function<void (AVFrame *)> c)
{
    if(c) mux_option_.setCamera_callback(c);
}

void recoder::set_stop_callback(std::function<void ()> c)
{
    if(c) mux_option_.setStop_callback(c);
}
