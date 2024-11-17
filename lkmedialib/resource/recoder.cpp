#include "recoder.h"
#include "thread"
#include "windows.h"
#include "eventCapturer.h"
#include "common.h"

int add_video_stream(AVFormatContext* format_context,
                     AVStream** out_stream,
                     AVCodecContext** codec_context,
                     AVCodec** codec,
                     AVCodecID codec_id){

    *out_stream = avformat_new_stream(format_context, nullptr);
    if(!(*out_stream)){
        RootError() << "avformat_new_stream failed";
        return -1;
    }

    *codec = avcodec_find_encoder(codec_id);
    if(!(*codec)){
        RootError() << "avcodec_find_encoder for AV_CODEC_ID_H264 failed";
        return -1;
    }

    *codec_context = avcodec_alloc_context3(*codec);
    if(!(*codec_context)){
        RootError() << "avcodec_alloc_context3 failed";
        return -1;
    }

    return 0;
}

void log_mux_option(mux_option mo){
    RootTrace() << "save path: " << mo.save_file_path();
    RootTrace() << "camera flag: " << mo.grab_option().has_camera();
    RootTrace() << "window flag: " << mo.grab_option().has_window();
    RootTrace() << "audio flag: " << mo.has_audio();
    AVRational rate = mo.grab_option().getFrame_rate();
    RootTrace() << "frame rate: " << rate.num << "/" << rate.den;
}

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
    grab_option go;
    go.setHas_window(has_window);
    go.setHas_camera(has_camera);
    go.setFrame_rate(frame_rate);
    go.setDest_camera_fmt(AV_PIX_FMT_RGB24);
    go.setDest_window_fmt(AV_PIX_FMT_RGB24);
    mux_option_.setGrab_option(go);
    mux_option_.setHas_audio(has_audio);
    mux_option_.setMux_frame(mux_frame);
    set_video_callback(video_callback);
    set_camera_callback(camera_callback);
    set_stop_callback(stop_callback);
    recording_ = false;
    consumer_exit_flag_ = false;
}

void recoder::record()
{
    if(!recording_){
        recording_ = true;
        if(mux_option_.grab_option().has_window()){
            graber_.init(mux_option_.grab_option(),
            [this](AVFrame* frame){ /// window callback
                int err;
                auto frameYuv = av_frame_alloc();
                frameYuv->height = frame->height > 1080 ? 1080 : frame->height;
                frameYuv->width = frame->width > 1920 ? 1920 : frame->width;
                frameYuv->format = AV_PIX_FMT_YUV420P;
                err = av_frame_get_buffer(frameYuv, 32);
                if(err < 0){
                    RootError() << "av_frame_get_buffer failed to alloc memory for frameYuv";
                    exit(0);
                }
                static SwsContext* sws_scaler_window = \
                        sws_getContext(frame->width,
                                       frame->height,
                                       AV_PIX_FMT_RGB24,
                                       frameYuv->width,
                                       frameYuv->height,
                                       AV_PIX_FMT_YUV420P,
                                       SWS_BILINEAR, nullptr, nullptr, nullptr);
                err = sws_scale(sws_scaler_window,
                                frame->data,
                                frame->linesize,
                                0, frame->height,
                                frameYuv->data,
                                frameYuv->linesize);
                if (err <= 0) {
                    RootError() << "sws_scale_window failed";
                    exit(0);
                }
                frameYuv->pts = frame->pts;
                video_frame_queue_.put(frameYuv);
//                av_frame_unref(frameYuv);
//                av_frame_free(&frameYuv);
                if(mux_option_.video_callback()){
                    mux_option_.video_callback()(frame);
                }
            },
            [this](AVFrame* frame){ /// camera callback
                if(mux_option_.camera_callback()){
                    mux_option_.camera_callback()(frame);
                }
            },
            [this](){ /// stop callback
                if(mux_option_.stop_callback()){
                    mux_option_.stop_callback()();
                }
            });

            graber_.grab();
        }else if(mux_option_.grab_option().has_camera()){
            graber_.init(mux_option_.grab_option(),
                         nullptr,
            [this](AVFrame* frame){ /// camera_callback
                int err;
                auto frameYuv = av_frame_alloc();
                frameYuv->height = frame->height;
                frameYuv->width = frame->width;
                frameYuv->format = AV_PIX_FMT_YUV420P;
                err = av_frame_get_buffer(frameYuv, 32);
                if(err < 0){
                    RootError() << __FILE__ << " av_frame_get_buffer failed to alloc memory for frameYuv";
                    exit(0);
                }
                static SwsContext* sws_scaler_camera = \
                        sws_getContext(frame->width,
                                       frame->height,
                                       AV_PIX_FMT_RGB24,
                                       frame->width,
                                       frame->height,
                                       AV_PIX_FMT_YUV420P,
                                       SWS_BILINEAR, nullptr, nullptr, nullptr);
                int ret = sws_scale(sws_scaler_camera,
                                    frame->data,
                                    frame->linesize,
                                    0, frame->height,
                                    frameYuv->data,
                                    frameYuv->linesize);
                if (ret <= 0) {
                    RootError() << "sws_scale_camera failed";
                    exit(0);
                }

                frameYuv->pts = frame->pts;

                video_frame_queue_.put(frameYuv);
                if(mux_option_.camera_callback()){
                    mux_option_.camera_callback()(frame);
                }
            },
            [this](){
                if(mux_option_.stop_callback()){
                    mux_option_.stop_callback()();
                }
            });

            graber_.grab();
        }

        // 主要用于消费队列中的帧，将其复用到save_file_path中
        std::thread consumer_thread([this](){
            consumer();
        });

        consumer_thread.detach();
    }
}

void recoder::stop()
{
    if(recording_){
        recording_ = false;
        graber_.stop();
        // 然后在等待consumer线程结束
        std::unique_lock<std::mutex> lock(consumer_exit_mtx_);
        if(!consumer_exit_flag_)
            consumer_exit_cond_.wait(lock, [this](){return consumer_exit_flag_;});
        consumer_exit_flag_ = false;
        RootTrace() << "recoder stopped";
    }
}

void recoder::consumer()
{
    int err;
    int screen_width = GetSystemMetrics(SM_CXSCREEN); // 32位下，最大为1920
    int screen_height = GetSystemMetrics(SM_CYSCREEN); // 32位下，最大为1080
    AVFormatContext *o_fmt_ctx;

    AVStream *o_stream_a = nullptr;
    AVCodec *coder_a = nullptr;
    AVCodecContext *coder_ctx_a = nullptr;

    AVStream *o_stream_v = nullptr;
    AVCodec *coder_v = nullptr;
    AVCodecContext *coder_ctx_v = nullptr;

    AVFrame* frame = nullptr;

    // Trace 打印 mux_option
    log_mux_option(mux_option_);
    // 设置输出文件路径
    std::string tmp_path = mux_option_.save_file_path();
    const std::string output_path = tmp_path.empty() ? "./output.mp4" : tmp_path;
    // 申请输出格式sha上下文
    err = avformat_alloc_output_context2(&o_fmt_ctx, nullptr, "mp4", tmp_path.c_str());
    if(err < 0){
        RootError() << "avformat_alloc_output_context2 failed with: ";
        exit(0);
    }

    // 根据用户设置，初始化输出格式上下文、码流、编码器
    if(mux_option_.has_audio()){
        o_stream_a = avformat_new_stream(o_fmt_ctx, nullptr);
        if(!o_stream_a){
            RootError() << "avformat_new_stream failed";
            exit(0);
        }
        coder_a = avcodec_find_encoder(AV_CODEC_ID_AAC);
        if(!coder_a){
            RootError() << "avcodec_find_encoder for AV_CODEC_ID_AAC failed";
            exit(0);
        }
        coder_ctx_a = avcodec_alloc_context3(coder_a);
        if(!coder_ctx_a){
            RootError() << "avcodec_alloc_context3 for coder_a failed";
            exit(0);
        }

        // 设置编码器参数
        coder_ctx_a->sample_rate = 44100;
        coder_ctx_a->channel_layout = AV_CH_LAYOUT_STEREO;
        coder_ctx_a->channels = av_get_channel_layout_nb_channels(coder_ctx_a->channel_layout);
        coder_ctx_a->sample_fmt = coder_a->sample_fmts[0];
        coder_ctx_a->time_base = {1, coder_ctx_a->sample_rate};


        // 复制编码器参数到流对应码流中
        err = avcodec_parameters_from_context(o_stream_a->codecpar, coder_ctx_a);
        if(err < 0){
            RootError() << "avcodec_parameters_from_context failed with: ";
            exit(0);
        }

        // 打开编码器
        if(avcodec_open2(coder_ctx_a, coder_a, nullptr) < 0){
            RootError() << "avcodec_open2 for audio failed";
            exit(0);
        }
    }
    if(mux_option_.grab_option().has_window()){
        RootTrace() << "initialized window related resource";
        err = add_video_stream(o_fmt_ctx,
                               &o_stream_v,
                               &coder_ctx_v,
                               &coder_v,
                               AV_CODEC_ID_H264);
        if(err < 0){
            avformat_free_context(o_fmt_ctx);
            RootError() << "add_video_stream for video stream failed";
            exit(0);
        }
        // 设置编码器参数(32位编译下，1600*2560分辨率会因为内存分配限制而导致avcodec_open2失败)
        coder_ctx_v->gop_size = 1;
        coder_ctx_v->width = 1920;
        coder_ctx_v->height = 1080;
        coder_ctx_v->bit_rate = 12000000;
        coder_ctx_v->codec_id = AV_CODEC_ID_H264;
        coder_ctx_v->pix_fmt = AV_PIX_FMT_YUV420P;
        coder_ctx_v->framerate = mux_option_.grab_option().getFrame_rate();
        coder_ctx_v->time_base = {1, mux_option_.grab_option().getFrame_rate().num};
        coder_ctx_v->has_b_frames = 0;
        coder_ctx_v->max_b_frames = 0;
        // 经过测试，如下这行必须写，不然写入的文件可能会因为格式不正确而无法播放
        if(o_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER){
            coder_ctx_v->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }
        // 打开编码器
        err = avcodec_open2(coder_ctx_v, coder_v, nullptr);
        if(err < 0){
            err2str("avcodec_open2 for video failed: ", err);
            exit(0);
        }
        // 复制编码器参数
        err = avcodec_parameters_from_context(o_stream_v->codecpar, coder_ctx_v);
        if(err < 0){
            RootError() << "avcodec_parameters_from_context failed for video";
            exit(0);
        }

    }else if(mux_option_.grab_option().has_camera()){
        RootTrace() << "initialized camera related resource";
        err = add_video_stream(o_fmt_ctx,
                               &o_stream_v,
                               &coder_ctx_v,
                               &coder_v,
                               AV_CODEC_ID_H264);
        if(err < 0){
            avformat_free_context(o_fmt_ctx);
            RootError() << "add_video_stream for video stream failed";
            exit(0);
        }
        // 设置编码器参数(32位编译下，1600*2560分辨率会因为内存分配限制而导致avcodec_open2失败)
        coder_ctx_v->height = graber_.getCamera_height();
        coder_ctx_v->width = graber_.getCamera_width();
        coder_ctx_v->framerate = mux_option_.grab_option().getFrame_rate();
        coder_ctx_v->time_base = {1, mux_option_.grab_option().getFrame_rate().num};
        coder_ctx_v->bit_rate = 4000000;
        coder_ctx_v->codec_id = AV_CODEC_ID_H264;
        coder_ctx_v->pix_fmt = AV_PIX_FMT_YUV420P;
        coder_ctx_v->gop_size = 12;
        // 经过测试，如下这行必须写，不然写入的文件可能会因为格式不正确而无法播放
        if(o_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER){
            coder_ctx_v->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        err = avcodec_open2(coder_ctx_v, coder_v, nullptr);
        if(err < 0){
            RootError() << "avcodec_open2 for camera failed";
            exit(0);
        }

        // 复制编码器参数
        err = avcodec_parameters_from_context(o_stream_v->codecpar, coder_ctx_v);
        if(err < 0){
            RootError() << "avcodec_parameters_from_context failed for camera";
            exit(0);
        }
    }

    // 打开输出文件（avio_open会根据情况自动设置内部码流的time_base，无需手动复制）
    if(!(o_fmt_ctx->oformat->flags & AVFMT_NOFILE)){
        if(avio_open(&o_fmt_ctx->pb, output_path.c_str(), AVIO_FLAG_WRITE) < 0){
            RootError() << "avio_open failed";
            exit(0);
        }
    }

    // 首先，将输出格式上下文中的头信息写入文件
    if((err = avformat_write_header(o_fmt_ctx, nullptr)) < 0){
        RootError() << "avformat_write_header failed with: ";
        exit(0);
    }
    RootDebug() << "process has written header to file: " << mux_option_.save_file_path();

    // 开始从队列读取帧数据，并进行相应的编码操作
    while(recording_ || !video_frame_queue_.empty()){
        frame = video_frame_queue_.get();
        show_frame_information(frame, AVMEDIA_TYPE_VIDEO);
        if(frame->pts == 200){
            break;
        }
        err = avcodec_send_frame(coder_ctx_v, frame);
        // 立马释放frame资源
        av_frame_unref(frame);
        av_frame_free(&frame);
        if(err < 0){
            if(err == AVERROR(EAGAIN)){
                RootDebug() << "Encoder not ready, try again";
                continue;
            } else if(err == AVERROR_EOF){
                // 编码结束，退出循环
                break;
            } else {
                err2str("avcodec_send_frame failed with error: ", err);
                exit(0);
            }
        }
        while (true) {
            AVPacket packet;
            av_init_packet(&packet);
            packet.data = nullptr;
            packet.size = 0;

            err = avcodec_receive_packet(coder_ctx_v, &packet);
            if (err == AVERROR(EAGAIN) || err == AVERROR_EOF) {
                err2str("", err);
                break;
            } else if (err < 0) {
                RootError() << "avcodec_receive_packet failed with error: ";
                exit(0);
            }

            // 设置 packet 的时间戳等信息
            av_packet_rescale_ts(&packet, coder_ctx_v->time_base, o_stream_v->time_base);
            packet.stream_index = o_stream_v->index;

            RootDebug() << "Encoder time base: " << coder_ctx_v->time_base.num << "/" << coder_ctx_v->time_base.den;
            RootDebug() << "Stream time base: " << o_stream_v->time_base.num << "/" << o_stream_v->time_base.den;

            if((err = av_interleaved_write_frame(o_fmt_ctx, &packet)) < 0){
                RootError() << "av_interleaved_write_frame failed with error: ";
                exit(0);
            }

            av_packet_unref(&packet);
        }
    }

    RootTrace() << "video_frame_queue_ and audio_frame_queue_ is empty";
    RootTrace() << "now we need to send nullptr packet to tell encoder mux all remain packets";
    avcodec_send_frame(coder_ctx_v, nullptr);
    RootTrace() << "NULL packet has been sent to encoder. it's time to mux all remian packets";
    while (true) {
        AVPacket packet;
        av_init_packet(&packet);
        packet.data = nullptr;
        packet.size = 0;

        err = avcodec_receive_packet(coder_ctx_v, &packet);
        if (err == AVERROR(EAGAIN) || err == AVERROR_EOF) {
            break;
        } else if (err < 0) {
            RootError() << "avcodec_receive_packet failed with error: ";
            exit(0);
        }

        // Rescale packet timestamps
        av_packet_rescale_ts(&packet, coder_ctx_v->time_base, o_stream_v->time_base);
        packet.stream_index = o_stream_v->index;

        if ((err = av_interleaved_write_frame(o_fmt_ctx, &packet)) < 0) {
            RootError() << "av_interleaved_write_frame failed with error: ";
            exit(0);
        }

        av_packet_unref(&packet);
    }
    RootTrace() << "after mux remain packets, we need write trailer to file and free all resource";
    av_write_trailer(o_fmt_ctx);

    // 释放资源
    avcodec_free_context(&coder_ctx_v);
    avio_close(o_fmt_ctx->pb);
    avformat_free_context(o_fmt_ctx);

    RootDebug() << "record ends";
    // mux 线程结束通知
    {
        std::unique_lock<std::mutex> lock(consumer_exit_mtx_);
        consumer_exit_flag_ = true;
        consumer_exit_cond_.notify_one();
    }
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

