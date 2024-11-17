#include "graber.h"
#include "thread"
#include "opencv2/opencv.hpp"
#include "QDebug"
#include "eventCapturer.h"
#include "windows.h"

extern "C"{
    #include "libavutil/time.h"
    #include "libavdevice/avdevice.h"
    #include "libavformat/avformat.h"
}

graber::graber():
    grab_option_({false, true, false, {25,1}}),
    record_flag_(false),
    camera_callback_(nullptr),
    video_callback_(nullptr),
    stop_callback_(nullptr),
    thread_exited_flag_(false),
    camera_width_(max_record_camera_width),
    camera_height_(max_record_camera_height),
    can_get_height_width_(false)
{

}

void graber::init(grab_option ro, \
                   std::function<void(AVFrame* frame)> video_callback, \
                   std::function<void(AVFrame* frame)> camera_callback, \
                   std::function<void(void)> stop_callback)
{
    grab_option_ = ro;
    video_callback_ = std::move(video_callback);
    camera_callback_ = std::move(camera_callback);
    stop_callback_ = std::move(stop_callback);
    record_flag_ = false;
    thread_exited_flag_ = false;
}

void graber::grab()
{
    if(!record_flag_){
        record_flag_ = true;
        std::thread video_record_thread([this](){
            int err;
            int failed_read_times = 0;
            int screen_width = GetSystemMetrics(SM_CXSCREEN);
            int screen_height = GetSystemMetrics(SM_CYSCREEN);
            int frame_index_camera = 0;
            int frame_index_window = 0;
            const uint8_t* mat_data[4] = {0, 0, 0, 0};
            int mat_linesize[4] = {0, 0, 0, 0};
            int really_camera_frame_height = 0;
            int really_camera_frame_width = 0;
            AVFrame* frameDST = av_frame_alloc();
            SwsContext* sws_camera_context = nullptr;
            bool camera_need_scale = grab_option_.getDest_camera_fmt() != AV_PIX_FMT_BGR24;
            bool window_need_scale = grab_option_.getDest_window_fmt() != AV_PIX_FMT_BGRA;
            cv::Mat mat;
            cv::VideoCapture cap;

            AVFrame* frameWin = av_frame_alloc();
            AVFrame* frameWinRGB = av_frame_alloc();
            AVPacket packet;
            AVInputFormat* window_input_format = nullptr;
            AVFormatContext* window_format_context = nullptr;
            AVStream* videoStream = nullptr;
            AVCodecParameters* codecpar = nullptr;
            AVCodec* codec = nullptr;
            AVCodecContext* codecContext = nullptr;
            SwsContext* sws_window_context = nullptr;

            if(grab_option_.has_window()){
                avdevice_register_all();
                window_input_format = av_find_input_format("gdigrab");
                if(avformat_open_input(&window_format_context, "desktop", window_input_format, nullptr) != 0){
                    qDebug() << "open window format context failed";
                    exit(0);
                }
                if(avformat_find_stream_info(window_format_context, nullptr) < 0){
                    qDebug() << "find window stream info failed";
                    exit(0);
                }
                for (unsigned int i = 0; i < window_format_context->nb_streams; i++) {
                    if (window_format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                        videoStream = window_format_context->streams[i];
                        break;
                    }
                }
                if (!videoStream) {
                    qDebug() << "No video stream found";
                    exit(0);
                }
                codecpar = videoStream->codecpar;
                codec = avcodec_find_decoder(codecpar->codec_id);
                if (!codec) {
                    qDebug() << "Codec not found";
                    exit(0);
                }
                codecContext = avcodec_alloc_context3(codec);
                if (avcodec_parameters_to_context(codecContext, codecpar) < 0) {
                    qDebug() << "Failed to copy codec parameters to codec context";
                    exit(0);
                }
                if (avcodec_open2(codecContext, codec, NULL) < 0) {
                    qDebug() << "Failed to open codec";
                    exit(0);
                }
                // 设置frameWin的参数并申请空间
                frameWin->height = screen_height;
                frameWin->width = screen_width;
                frameWin->format = AV_PIX_FMT_BGRA;
//                av_frame_get_buffer(frameWin, 32);
                // 设置frameWinRGB的参数并申请空间
                frameWinRGB->height = screen_height;
                frameWinRGB->width = screen_width;
                frameWinRGB->format = AV_PIX_FMT_RGB24;
                av_frame_get_buffer(frameWinRGB, 32);
                if(window_need_scale)
                    sws_window_context = sws_getContext(screen_width, screen_height,
                                                        AV_PIX_FMT_BGRA,
                                                        screen_width,screen_height,
                                                        grab_option_.getDest_window_fmt(),
                                                        SWS_BILINEAR, nullptr, nullptr, nullptr);
                show_codec_context_information(codec, codecContext, videoStream->index);
                RootTrace() << "grabber has initialized window related resource";
            }
            if(grab_option_.has_camera()){
                cap.open(0);
                if(!cap.isOpened()){
                    qDebug() << "cap(0) can not be opend";
                    exit(0);
                }
                cap.set(cv::CAP_PROP_FRAME_HEIGHT, max_record_camera_height);
                cap.set(cv::CAP_PROP_FRAME_WIDTH, max_record_camera_width);

                really_camera_frame_height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
                really_camera_frame_width = cap.get(cv::CAP_PROP_FRAME_WIDTH);

                { // 设置一下成员函数中camera的宽高，以供调用者使用
                    std::unique_lock<std::mutex> lock(height_width_mtx_);
                    camera_width_ = really_camera_frame_width;
                    camera_height_ = really_camera_frame_height;
                    can_get_height_width_ = true;
                    height_width_cond_.notify_one();
                }
                // 这里不能使用av_image_alloc，因为avimagealloc不会为frame申请引用计数
//                av_image_alloc(frameDST->data,
//                               frameDST->linesize,
//                               really_camera_frame_width,
//                               really_camera_frame_height,
//                               grab_option_.getDest_camera_fmt(),
//                               32);
                // 首先设计frame的字段
                frameDST->height = really_camera_frame_height;
                frameDST->width = really_camera_frame_width;
                frameDST->format = grab_option_.getDest_camera_fmt();

                // 申请可引用的frame
                err = av_frame_get_buffer(frameDST, 32);
                if(err < 0){
                    qDebug() << __FILE__ << " av_frame_get_buffer failed";
                    exit(0);
                }

                if(camera_need_scale)
                    sws_camera_context = sws_getContext(really_camera_frame_width,
                                                really_camera_frame_height,
                                                AV_PIX_FMT_BGR24,
                                                really_camera_frame_width,
                                                really_camera_frame_height,
                                                grab_option_.getDest_camera_fmt(),
                                                SWS_BILINEAR, nullptr, nullptr, nullptr);
                RootTrace() << "grabber has initialized camera related resource";
            }

            while(record_flag_){
                if(grab_option_.has_camera()){
                    if(!cap.read(mat)){
                        if(failed_read_times++ == 10){
                            RootWarn() << "截取摄像头失败";
                        }
                        continue;
                    }

                    cv::flip(mat, mat, 1);

                    mat_data[0] = mat.data;
                    mat_linesize[0] = static_cast<int>(mat.step);

                    // 将mat中的数据转换成 dest_video_information_ 期盼的格式
                    if(camera_need_scale)
                        err = sws_scale(sws_camera_context,
                                        mat_data,
                                        mat_linesize,
                                        0, really_camera_frame_height,
                                        frameDST->data,
                                        frameDST->linesize);
                    else{
                        av_image_copy(frameDST->data,
                                      frameDST->linesize,
                                      mat_data,
                                      mat_linesize,
                                      AV_PIX_FMT_BGR24,
                                      really_camera_frame_width,
                                      really_camera_frame_height);
                    }
                    frameDST->pts = frame_index_camera++;
                    if(camera_callback_) camera_callback_(frameDST);
                }
                if(grab_option_.has_window()){
                    err = av_read_frame(window_format_context, &packet);
                    if(err < 0){
                        err2str("av_read_frame failed with: ", err);
//                        exit(0);
                        av_packet_unref(&packet);
                        continue;
                    }
                    if(packet.stream_index == videoStream->index){
                        int ret = avcodec_send_packet(codecContext, &packet);
                        if (ret < 0) {
                            RootError() << "Error sending packet for decoding";
                            exit(0);
                        }
                        // 从解码器接收解码后的 frame
                        while(ret >= 0){
                            ret = avcodec_receive_frame(codecContext, frameWin);
                            if (ret == AVERROR(EAGAIN)) {
                                break;
                            }
                            if(ret == AVERROR_EOF){
                                break;
                            }
                            if (ret < 0) {
                                RootError() << "avcodec_receive_frame failed";
                                exit(0);
                            }
                            if(window_need_scale)
                                sws_scale(sws_window_context,
                                          frameWin->data,
                                          frameWin->linesize,
                                          0, codecContext->height,
                                          frameWinRGB->data,
                                          frameWinRGB->linesize);
                            frameWinRGB->pts = frame_index_window++;
                            if(video_callback_) video_callback_(frameWinRGB);
                            av_frame_unref(frameWin);
                        }

                    }
                    av_packet_unref(&packet);
                }

                AVRational frame_rate = grab_option_.getFrame_rate();
                unsigned int micro_second_sleep_time = static_cast<unsigned int>((double)frame_rate.den / frame_rate.num * 1000 * 1000);
                av_usleep(micro_second_sleep_time);
            }

            // 清理资源结束录制
            if(grab_option_.has_window()){
                av_packet_unref(&packet);
                av_frame_unref(frameWin);
                av_frame_free(&frameWin);
                av_frame_unref(frameWinRGB);
                av_frame_free(&frameWinRGB);
                avformat_close_input(&window_format_context);
                avcodec_free_context(&codecContext);
                if(window_need_scale) sws_freeContext(sws_window_context);
            }

            if(grab_option_.has_camera()){
                sws_freeContext(sws_camera_context);
                av_frame_free(&frameDST);
                cap.release(); // 其实在cap析构的时候，也会自动调用release的，所以这里可以不用release
                if(camera_need_scale) sws_freeContext(sws_camera_context);
            }

            // 设置退出标志
            {
                std::unique_lock<std::mutex> lock(thread_exited_flag_mtx);
                thread_exited_flag_ = true;
                thread_exited_flag_cond_.notify_one();
            }
        });
        video_record_thread.detach();
    }
}

void graber::stop()
{
    if(record_flag_){
        std::unique_lock<std::mutex> lock(thread_exited_flag_mtx);
        record_flag_ = false;
        thread_exited_flag_cond_.wait(lock, [this](){
           return thread_exited_flag_;
        });
        thread_exited_flag_ = false;
        if(stop_callback_) stop_callback_();
        RootTrace() << "grabber stopped";
    }
}

int graber::getCamera_width()
{
    std::unique_lock<std::mutex> lock(height_width_mtx_);
    if(!can_get_height_width_){
        height_width_cond_.wait(lock, [this](){
            return can_get_height_width_;
        });
    }
    return camera_width_;
}

void graber::setCamera_width(int value)
{
    camera_width_ = value;
}

int graber::getCamera_height()
{
    std::unique_lock<std::mutex> lock(height_width_mtx_);
    if(!can_get_height_width_){
        height_width_cond_.wait(lock, [this](){
            return can_get_height_width_;
        });
    }
    return camera_height_;
}

void graber::setCamera_height(int value)
{
    camera_height_ = value;
}

