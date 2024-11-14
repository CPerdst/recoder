#include "graber.h"
#include "thread"
#include "opencv2/opencv.hpp"
#include "QDebug"

#include "windows.h"

extern "C"{
    #include "libavutil/time.h"
    #include "libavdevice/avdevice.h"
    #include "libavformat/avformat.h"
}

graber::graber():
    record_option_({false, true, false, {25,1}}),
    record_flag_(false),
    camera_callback_(nullptr),
    video_callback_(nullptr),
    stop_callback_(nullptr),
    thread_exited_flag_(false)
{

}

void graber::init(grab_option ro, \
                   std::function<void(AVFrame* frame)> video_callback, \
                   std::function<void(AVFrame* frame)> camera_callback, \
                   std::function<void(void)> stop_callback)
{
    record_option_ = ro;
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
            const uint8_t* mat_data[4] = {0, 0, 0, 0};
            int mat_linesize[4] = {0, 0, 0, 0};
            int really_camera_frame_height = 0;
            int really_camera_frame_width = 0;
            AVFrame* frameDST = av_frame_alloc();
            SwsContext* sws_camera_context;
            cv::Mat mat;
            cv::VideoCapture cap;

            AVFrame* frameWin = av_frame_alloc();
            AVFrame* frameWinRGB = av_frame_alloc();
            AVPacket packet;
            AVInputFormat* window_input_format;
            AVFormatContext* window_format_context;
            AVStream* videoStream = NULL;
            AVCodecParameters* codecpar;
            AVCodec* codec;
            AVCodecContext* codecContext;
            SwsContext* sws_window_context;

            if(record_option_.has_window()){
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
                av_image_alloc(frameWin->data,
                               frameWin->linesize,
                               screen_width,
                               screen_height,
                               AV_PIX_FMT_BGRA,
                               32);
                av_image_alloc(frameWinRGB->data,
                               frameWinRGB->linesize,
                               screen_width,
                               screen_height,
                               AV_PIX_FMT_RGB24,
                               32);
                sws_window_context = sws_getContext(screen_width, screen_height,
                                                    AV_PIX_FMT_BGRA,
                                                    screen_width,screen_height,
                                                    AV_PIX_FMT_RGB24,
                                                    SWS_BILINEAR, nullptr, nullptr, nullptr);
                show_codec_context_information(codec, codecContext, videoStream->index);
            }

            if(record_option_.has_camera()){
                cap.open(0);
                if(!cap.isOpened()){
                    qDebug() << "cap(0) can not be opend";
                    exit(0);
                }
                cap.set(cv::CAP_PROP_FRAME_HEIGHT, max_record_camera_height);
                cap.set(cv::CAP_PROP_FRAME_WIDTH, max_record_camera_width);

                really_camera_frame_height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
                really_camera_frame_width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
                av_image_alloc(frameDST->data,
                               frameDST->linesize,
                               really_camera_frame_width,
                               really_camera_frame_height,
                               record_option_.getDest_camera_fmt(),
                               32);
                sws_camera_context = sws_getContext(really_camera_frame_width,
                                            really_camera_frame_height,
                                            AV_PIX_FMT_BGR24,
                                            really_camera_frame_width,
                                            really_camera_frame_height,
                                            record_option_.getDest_camera_fmt(),
                                            SWS_BILINEAR, nullptr, nullptr, nullptr);
            }

            while(record_flag_){
                if(record_option_.has_camera()){
                    if(!cap.read(mat)){
                        if(failed_read_times++ == 10){
                            qDebug() << "截取摄像头失败";
                        }
                        continue;
                    }

                    cv::flip(mat, mat, 1);

                    frameDST->width = really_camera_frame_width;
                    frameDST->height = really_camera_frame_height;

                    mat_data[0] = mat.data;
                    mat_linesize[0] = static_cast<int>(mat.step);

                    // 将mat中的数据转换成 dest_video_information_ 期盼的格式
                    err = sws_scale(sws_camera_context,
                                    mat_data,
                                    mat_linesize,
                                    0, really_camera_frame_height,
                                    frameDST->data,
                                    frameDST->linesize);
                    if(camera_callback_) camera_callback_(frameDST);
                }
                if(record_option_.has_window()){
                    err = av_read_frame(window_format_context, &packet);
                    if(err < 0){
                        qDebug() << "av_read_frame failed";
                        exit(0);
                    }
                    if(packet.stream_index == videoStream->index){
                        int ret = avcodec_send_packet(codecContext, &packet);
                        if (ret < 0) {
                            qDebug() << "Error sending packet for decoding";
                            exit(0);
                        }
                        // 从解码器接收解码后的 frame
                        ret = avcodec_receive_frame(codecContext, frameWin);
                        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                            break; // 没有更多的帧或者 EOF
                        }
                        if (ret < 0) {
                            qDebug() << "avcodec_receive_frame failed";
                            exit(0);
                        }
                        sws_scale(sws_window_context,
                                  frameWin->data,
                                  frameWin->linesize,
                                  0, codecContext->height,
                                  frameWinRGB->data,
                                  frameWinRGB->linesize);
                        frameWinRGB->width = codecContext->width;
                        frameWinRGB->height = codecContext->height;
                        if(video_callback_) video_callback_(frameWinRGB);
                        av_packet_unref(&packet);
                    }
                }

                AVRational frame_rate = record_option_.getFrame_rate();
                unsigned int micro_second_sleep_time = static_cast<unsigned int>((double)frame_rate.den / frame_rate.num * 1000 * 1000);
                av_usleep(micro_second_sleep_time);
            }

            // 清理资源结束录制
            if(record_option_.has_window()){
                av_packet_unref(&packet);
                av_frame_unref(frameWin);
                av_frame_free(&frameWin);
                av_frame_unref(frameWinRGB);
                av_frame_free(&frameWinRGB);
                avformat_close_input(&window_format_context);
                avcodec_free_context(&codecContext);
            }

            if(record_option_.has_camera()){
                sws_freeContext(sws_camera_context);
                av_frame_free(&frameDST);
                cap.release(); // 其实在cap析构的时候，也会自动调用release的，所以这里可以不用release
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
    }
}

