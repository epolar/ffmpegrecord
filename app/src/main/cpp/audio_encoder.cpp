//
// Created by eraise on 2017/11/2.
//
#include "audio_encoder.h"

int audio_encoder::init(Arguments* vargs) {
    LOGD(DEBUG, "init audio encoder")
    arguments = vargs;
    LOGD(DEBUG, "av_register_all for audio")
    // 注册 ffmpeg 所有的编解码器
    av_register_all();
    av_log_set_callback(logcallback);

    char base_url[strlen(arguments->base_url)];
    strcpy(base_url, arguments->base_url);
    char* output_url = strcat(base_url, arguments->audio_name);
    LOGI(DEBUG, "audio output_url == %s", output_url);

    LOGD(DEBUG, "avformat_alloc_context")
    pFormatCtx = avformat_alloc_context();
    LOGD(DEBUG, "avformat_alloc_output_context2");
//    avformat_alloc_output_context2(&pFormatCtx, NULL, NULL, vargs->audio_name);
//    fmt = pFormatCtx->oformat;

    fmt = av_guess_format(NULL, output_url, NULL);
    pFormatCtx->oformat = fmt;

    if (avio_open(&pFormatCtx->pb, output_url, AVIO_FLAG_READ_WRITE) < 0) {
        LOGE(DEBUG, "写入文件打开失败");
        return -1;
    }

    av_dump_format(pFormatCtx, 0, output_url, 1);

    audio_st = avformat_new_stream(pFormatCtx, 0);
    if (audio_st == NULL) {
        return -1;
    }
//
//    pCodecCtx = audio_st->codec;
//    pCodecCtx->codec_id = fmt->audio_codec;
//    pCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO;
//    pCodecCtx->sample_fmt = AV_SAMPLE_FMT_S16;
////    pCodecCtx->bit_rate = 128000;
//    pCodecCtx->sample_rate = arguments->audio_sample_rate;
//    pCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
//    pCodecCtx->channels = av_get_channel_layout_nb_channels(pCodecCtx->channel_layout);
////    pCodecCtx->time_base.num = 1;
////    pCodecCtx->time_base.den = 25;
////    pCodecCtx->thread_count = 4;
//
////    pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
//    if (!pCodec) {
//        LOGE(DEBUG, "未发现合适的音频编码器!!");
//        return -1;
//    }
//    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
//        LOGE(DEBUG, "音频编码器打开失败!!");
//        return -1;
//    }

    pCodec = avcodec_find_encoder_by_name("libfdk_aac");
    if (!pCodec) {
        LOGD(DEBUG, "未发现音频编码器！")
        return -1;
    }
    pCodecCtx = avcodec_alloc_context3(pCodec);
    if (!pCodecCtx) {
        LOGD(DEBUG, "音频编码器打开失败！！");
        return -1;
    }
    pCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO;
    pCodecCtx->sample_fmt = AV_SAMPLE_FMT_S16;
    pCodecCtx->sample_rate = arguments->audio_sample_rate;
    pCodecCtx->bit_rate = 128000;
    pCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
    pCodecCtx->channels = av_get_channel_layout_nb_channels(pCodecCtx->channel_layout);
    pCodecCtx->time_base = {1, 1000};

    int ret = avcodec_open2(pCodecCtx, NULL, NULL);
    if (ret != 0) {
        char buf[1024] = {0};
        av_strerror(ret, buf, sizeof(buf));
        LOGD(DEBUG, "音频编码器打开失败: %s", buf);
        return -1;
    }

    pFrame = av_frame_alloc();
    pFrame->nb_samples = pCodecCtx->frame_size;
    pFrame->format = pCodecCtx->sample_fmt;
    pFrame->channels = pCodecCtx->channels;
    pFrame->channel_layout = pCodecCtx->channel_layout;
    pFrame->sample_rate = pCodecCtx->sample_rate;

    size = av_samples_get_buffer_size(NULL,
                                      pCodecCtx->channels,
                                      pCodecCtx->frame_size,
                                      pCodecCtx->sample_fmt,
                                      1);
//    frame_buf = (uint8_t *)av_malloc((size_t)size);
//    avcodec_fill_audio_frame(pFrame, pCodecCtx->channels, pCodecCtx->sample_fmt, frame_buf, size, 1);

    avformat_write_header(pFormatCtx, NULL);

//    av_new_packet(&pkt, size);

    // 编码线程
    pthread_create(&encode_thread, NULL, audio_encoder::start_encode, this);

    LOGD(DEBUG, "音频编码初始化完成")

    return 0;

}

int audio_encoder::get_frame_size() {
    return size;
}

void audio_encoder::push_frame(uint8* srcData, int len) {
    processing = true;
//    size_t msize = sizeof(srcData) * len;
    uint8* frame = (uint8 *) malloc((size_t)len);
    memcpy(frame, srcData, (size_t)len);
    if (start_time == 0) {
        start_time = utils::getCurrentTime();
    }
    audio_pcm_data data = {len, utils::getCurrentTime() - start_time, frame};
    queue.push(data);
    processing = false;
}

void audio_encoder::encode_frame(audio_pcm_data *pcm_data) {
    int ret = 0;
    uint8 *srcData = pcm_data->data;
    ret = avcodec_fill_audio_frame(pFrame,
                                   pCodecCtx->channels,
                                   pCodecCtx->sample_fmt,
                                   srcData, (int)pcm_data->len, 0);
//    pFrame->pts = frame_count ++;
    ret = avcodec_send_frame(pCodecCtx, pFrame);
    if (ret != 0) {
        LOGD(DEBUG, "send frame failed")
    }

    while (avcodec_receive_packet(pCodecCtx, &pkt) >= 0) {
        av_write_frame(pFormatCtx, &pkt);
        av_packet_unref(&pkt);
    }


//    pFrame->data[0] = srcData;
//    av_new_packet(&pkt, size);
//    pkt.data = NULL;
//    pkt.size = 0;
//    // 该帧的时间
////    pFrame->pts = (utils::getCurrentTime() - arguments->start_time) / arguments->audio_sample_rate;
////    pFrame->pts = frame_count ++;
//    int got_frame = 0;
//    int ret = avcodec_encode_audio2(pCodecCtx, &pkt, pFrame, &got_frame);
//    if (ret < 0) {
//        LOGE(DEBUG, "音频编码失败");
//        return;
//    }
//    if (got_frame == 1) {
////        LOGD(DEBUG, "成功编码 1 帧音频! \t size:%5d\n", pkt.size);
////        pkt.pts = av_rescale_q_rnd(pFrame->pts,
////                                   audio_st->codec->time_base,
////                                   audio_st->time_base,
////                                   AV_ROUND_NEAR_INF);
//        pkt.stream_index = audio_st->index;
//        ret = av_write_frame(pFormatCtx, &pkt);
//    } else {
//        LOGD(DEBUG, "skip a frame audio");
//    }
//    av_free_packet(&pkt);
}

void audio_encoder::stop() {
    isEnd = true;
//    LOGD(DEBUG, "flush_audio_encoder");
//    int ret = flush_encoder(0);
//    if (ret < 0) {
//        LOGE(DEBUG, "flush encoder audio failed\n");
//    }
//    LOGD(DEBUG, "av_write_trailer");
//    av_write_trailer(pFormatCtx);
//    if (audio_st) {
//        avcodec_close(audio_st->codec);
//        av_free(pFrame);
//        av_free(frame_buf);
//    }
//    avio_close(pFormatCtx->pb);
//    avformat_free_context(pFormatCtx);
//
//    LOGD(DEBUG, "End of audio encode");
}

int audio_encoder::flush_encoder(unsigned int stream_index) {
    int ret;
    int got_frame;
    AVPacket enc_pkt;
    if (!(pCodecCtx->codec->capabilities & CODEC_CAP_DELAY)) {
        return 0;
    }
    while (1) {
        enc_pkt.data = NULL;
        enc_pkt.size = 0;
        av_init_packet(&enc_pkt);
        ret = avcodec_encode_audio2(pCodecCtx, &enc_pkt, NULL, &got_frame);
        av_frame_free(NULL);
        if (ret < 0) {
            break;
        }
        if (!got_frame) {
            break;
        }
        LOGD(DEBUG, "flush encoder: successed to encode 1 frame audio! \tsize:%5d\n", enc_pkt.size);
        ret = av_write_frame(pFormatCtx, &enc_pkt);
        if (ret < 0) {
            break;
        }
        av_packet_unref(&enc_pkt);
    }
    return ret;
}

void *audio_encoder::start_encode(void *obj) {
    audio_encoder *encoder = (audio_encoder *)obj;
    while (!encoder->isEnd || !encoder->queue.empty()) {
        if (encoder->queue.empty()) {
            continue;
        }
        audio_pcm_data frame_buf = *encoder->queue.wait_and_pop().get();
        if (&frame_buf) {
            encoder->encode_frame(&frame_buf);
            free(frame_buf.data);
            free(&frame_buf);
        }
    }

    // 刷新缓存区数据
    if (encoder->flush_encoder(0) < 0) {
        LOGE(DEBUG, "Flushing encoder failed");
    }

    av_write_trailer(encoder->pFormatCtx);

    // 清空无用内存
    if (encoder->audio_st){
        avcodec_close(encoder->audio_st->codec);
        av_free(encoder->pFrame);
//        av_free(encoder->frame_buf);
    }
    avio_close(encoder->pFormatCtx->pb);
    avformat_free_context(encoder->pFormatCtx);

    encoder = NULL;
    LOGD(DEBUG, "End of video encode")
    return NULL;
}
