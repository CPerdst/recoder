#include "recoder.h"
#include "thread"
#include "windows.h"
#include "eventCapturer.h"

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
}

void recoder::record()
{
    if(mux_option_.grab_option().has_window()){
        graber_.init(mux_option_.grab_option(),
        [this](AVFrame* frame){
            auto f = av_frame_alloc();
            av_frame_ref(f, frame);
            video_frame_queue_.put(f);
            if(mux_option_.video_callback()){
                mux_option_.video_callback()(frame);
            }
            return;
        },
        [this](AVFrame* frame){
            if(mux_option_.camera_callback()){
                mux_option_.camera_callback()(frame);
            }
            return;
        },
        [this](){
            if(mux_option_.stop_callback()){
                mux_option_.stop_callback()();
            }
        });

        graber_.grab();
    }else if(mux_option_.grab_option().has_camera()){
        graber_.init(mux_option_.grab_option(),
                     nullptr,
        [this](AVFrame* frame){
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
            SwsContext* sws_scaler = \
                    sws_getContext(frame->width,
                                   frame->height,
                                   AV_PIX_FMT_RGB24,
                                   frame->width,
                                   frame->height,
                                   AV_PIX_FMT_YUV420P,
                                   SWS_BILINEAR, nullptr, nullptr, nullptr);
            int ret = sws_scale(sws_scaler,
                                frame->data,
                                frame->linesize,
                                0, frame->height,
                                frameYuv->data,
                                frameYuv->linesize);
            if (ret <= 0) {
                RootError() << "sws_scale failed";
                exit(0);
            }

            frameYuv->pts = frame->pts;

            video_frame_queue_.put(frameYuv);
            if(mux_option_.camera_callback()){
                mux_option_.camera_callback()(frame);
            }
            return;
        },
        [this](){
            if(mux_option_.stop_callback()){
                mux_option_.stop_callback()();
            }
        });

        graber_.grab();
    }

    std::thread consumer_thread([this](){
        consumer();
    });

    consumer_thread.detach();
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

void recoder::consumer()
{
    int err;
    int screen_width = GetSystemMetrics(SM_CXSCREEN); // 32位下，最大为1920
    int screen_height = GetSystemMetrics(SM_CYSCREEN); // 32位下，最大为1080
    AVFormatContext *o_fmt_ctx;

    int o_stream_a_idx = -1;
    AVStream *o_stream_a = nullptr;
    AVCodec *coder_a = nullptr;
    AVCodecContext *coder_ctx_a = nullptr;

    int o_stream_v_idx = -1;
    AVStream *o_stream_v = nullptr;
    AVCodec *coder_v = nullptr;
    AVCodecContext *coder_ctx_v = nullptr;

    std::string tmp_path = mux_option_.save_file_path();
    const std::string output_path = tmp_path.empty() ? "./output.mp4" : tmp_path;
    RootDebug() << output_path.c_str();

    err = avformat_alloc_output_context2(&o_fmt_ctx, nullptr, "mp4", tmp_path.c_str());
    if(err < 0){
        RootError() << "avformat_alloc_output_context2 failed with: ";
        exit(0);
    }

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

        o_stream_a_idx = o_stream_a->index;
    }

    if(mux_option_.grab_option().has_window()){
        o_stream_v = avformat_new_stream(o_fmt_ctx, nullptr);
        if(!o_stream_v){
            RootError() << "avformat_new_stream failed";
            exit(0);
        }

        coder_v = avcodec_find_encoder(AV_CODEC_ID_H264);
        if(!coder_v){
            RootError() << "avcodec_find_encoder for AV_CODEC_ID_H264 failed";
            exit(0);
        }

        coder_ctx_v = avcodec_alloc_context3(coder_v);
        if(!coder_ctx_v){
            RootError() << "avcodec_alloc_context3 failed";
            exit(0);
        }

        // 设置编码器参数(32位编译下，1600*2560分辨率会因为内存分配限制而导致avcodec_open2失败)
        coder_ctx_v->height = 1080;
        coder_ctx_v->width = 1920;
        RootError() << "coder height: " << coder_ctx_v->height << " coder width: " << coder_ctx_v->width;
        coder_ctx_v->pix_fmt = AV_PIX_FMT_YUV420P;
        coder_ctx_v->framerate = {25, 1};
        coder_ctx_v->time_base = {1, 25};
        coder_ctx_v->bit_rate = 400000;

        // 打开编码器
        err = avcodec_open2(coder_ctx_v, coder_v, nullptr);
        if(err < 0){
            RootError() << "avcodec_open2 for video failed";
            exit(0);
        }

        // 复制编码器参数
        err = avcodec_parameters_from_context(o_stream_v->codecpar, coder_ctx_v);
        if(err < 0){
            RootError() << "avcodec_parameters_from_context failed for video";
            exit(0);
        }

        o_stream_v_idx = o_stream_v->index;

    }else{
        if(mux_option_.grab_option().has_camera()){
            o_stream_v = avformat_new_stream(o_fmt_ctx, nullptr);
            if(!o_stream_v){
                RootError() << "avformat_new_stream failed";
                exit(0);
            }

            coder_v = avcodec_find_encoder(AV_CODEC_ID_H264);
            if(!coder_v){
                RootError() << "avcodec_find_encoder for AV_CODEC_ID_H264 failed";
                exit(0);
            }

            coder_ctx_v = avcodec_alloc_context3(coder_v);
            if(!coder_ctx_v){
                RootError() << "avcodec_alloc_context3 failed";
                exit(0);
            }

            // 设置编码器参数(32位编译下，1600*2560分辨率会因为内存分配限制而导致avcodec_open2失败)
            coder_ctx_v->height = graber_.getCamera_height();
            coder_ctx_v->width = graber_.getCamera_width();
            RootDebug() << "coder height: " << coder_ctx_v->height << " coder width: " << coder_ctx_v->width;
            coder_ctx_v->pix_fmt = AV_PIX_FMT_YUV420P;
            coder_ctx_v->framerate = mux_option_.grab_option().getFrame_rate();
            coder_ctx_v->time_base = {1, 25};
            coder_ctx_v->bit_rate = 4000000;
            coder_ctx_v->codec_id = AV_CODEC_ID_H264;

            coder_ctx_v->gop_size = 4;

//            o_stream_v->id = o_fmt_ctx->nb_streams - 1;
//            coder_ctx_v->max_b_frames = 4; // Disable B-frames
//            coder_ctx_v->has_b_frames = 4; // Ensure no B-frames are used

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

            o_stream_v_idx = o_stream_v->index;
        }
    }

    // 打开输出文件
    if(!(o_fmt_ctx->oformat->flags & AVFMT_NOFILE)){
        if(avio_open(&o_fmt_ctx->pb, output_path.c_str(), AVIO_FLAG_WRITE) < 0){
            RootError() << "avio_open failed";
            exit(0);
        }
    }

    if((err = avformat_write_header(o_fmt_ctx, nullptr)) < 0){
        RootError() << "avformat_write_header failed with: ";
        exit(0);
    }

    RootDebug() << "o_stream_ctx time base: " << av_q2d(o_stream_v->time_base);

    AVFrame* frame;
    int frame_index = 0;
    int cur_pts_v = 0, cur_pts_a = 0;

    while(true){
        frame = video_frame_queue_.get();
        if(frame->pts == 200){
            break;
        }
        err = avcodec_send_frame(coder_ctx_v, frame);
        if(err < 0){
            if(err == AVERROR(EAGAIN)){
                RootDebug() << "Encoder not ready, try again";
                continue;
            } else if(err == AVERROR_EOF){
                // 编码结束，退出循环
                break;
            } else {
                RootError() << "avcodec_send_frame failed with error: ";
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
                break;
            } else if (err < 0) {
                RootError() << "avcodec_receive_packet failed with error: ";
                exit(0);
            }

            // 设置 packet 的时间戳等信息
//            packet.stream_index = o_stream_v_idx;
//            qDebug() << "before trans: "<< packet.pts << " " << packet.dts << " " << packet.duration;
//            packet.pts = av_rescale_q(packet.pts, coder_ctx_v->time_base, o_stream_v->time_base);
//            packet.dts = av_rescale_q(packet.dts, coder_ctx_v->time_base, o_stream_v->time_base);
//            packet.duration = av_rescale_q(packet.duration, coder_ctx_v->time_base, o_stream_v->time_base);
//            qDebug() << "after trans: "<< packet.pts << " " << packet.dts << " " << packet.duration;

            av_packet_rescale_ts(&packet, coder_ctx_v->time_base, o_stream_v->time_base);
//            packet.stream_index = o_stream_v_idx;

            RootDebug() << "Encoder time base: " << coder_ctx_v->time_base.num << "/" << coder_ctx_v->time_base.den;
            RootDebug() << "Stream time base: " << o_stream_v->time_base.num << "/" << o_stream_v->time_base.den;

            if((err = av_interleaved_write_frame(o_fmt_ctx, &packet)) < 0){
                RootError() << "av_interleaved_write_frame failed with error: ";
                exit(0);
            }

            av_packet_unref(&packet);
        }
        av_frame_unref(frame); // 释放或取消引用帧

    }

    // Signal the encoder that no more frames will be sent
    avcodec_send_frame(coder_ctx_v, nullptr);

    // Receive any remaining packets
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
        packet.stream_index = o_stream_v_idx;

        if ((err = av_interleaved_write_frame(o_fmt_ctx, &packet)) < 0) {
            RootError() << "av_interleaved_write_frame failed with error: ";
            exit(0);
        }

        av_packet_unref(&packet);
    }

    av_write_trailer(o_fmt_ctx);


    RootDebug() << "end";
    std::this_thread::sleep_for(std::chrono::seconds(2));
    exit(0);

}

