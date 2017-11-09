//
// Created by eraise on 2017/11/2.
//
#include <audio_encoder.h>

int audio_encoder::init(Arguments* vargs) {
    LOGD(DEBUG, "init audio encoder")
    arguments = vargs;
    LOGD(DEBUG, "av_register_all for audio")
    // 注册 ffmpeg 所有的编解码器
    av_register_all();
    LOGD(DEBUG, "avformat_alloc_context")
    pFormatCtx = avformat_alloc_context();
    av_log_set_callback(logcallback);

    LOGD(DEBUG, "avformat_alloc_output_context2");
    avformat_alloc_output_context2(&pFormatCtx, NULL, NULL, vargs->audio_name);
    fmt = pFormatCtx->oformat;

    char base_url[strlen(arguments->base_url)];
    strcpy(base_url, arguments->base_url);
    char* output_url = strcat(base_url, arguments->audio_name);
    LOGI(DEBUG, "audio output_url == %s", output_url);

    if (avio_open(&pFormatCtx->pb, output_url, AVIO_FLAG_READ_WRITE) < 0) {
        LOGE(DEBUG, "写入文件打开失败");
        return -1;
    }

    audio_st = avformat_new_stream(pFormatCtx, 0);
    if (audio_st == NULL) {
        return -1;
    }

    pCodecCtx = audio_st->codec;
    pCodecCtx->codec_id = fmt->audio_codec;
    pCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO;
    pCodecCtx->sample_fmt = AV_SAMPLE_FMT_S16;
    pCodecCtx->sample_rate = arguments->audio_sample_rate;
    pCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
    pCodecCtx->channels = av_get_channel_layout_nb_channels(pCodecCtx->channel_layout);
    pCodecCtx->bit_rate = 64000;
    pCodecCtx->time_base.num = 1000;
    pCodecCtx->time_base.den = arguments->audio_sample_rate;

    av_dump_format(pFormatCtx, 0, output_url, 1);

    pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
    if (!pCodec) {
        LOGE(DEBUG, "未发现合适的音频编码器!!");
        return -1;
    }
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        LOGE(DEBUG, "音频编码器打开失败!!");
        return -1;
    }

    pFrame = av_frame_alloc();
    pFrame->nb_samples = pCodecCtx->frame_size;
    pFrame->format = pCodecCtx->sample_fmt;

    size = av_samples_get_buffer_size(NULL,
                                      pCodecCtx->channels,
                                      pCodecCtx->frame_size,
                                      pCodecCtx->sample_fmt,
                                      1);
    frame_buf = (uint8_t *)av_malloc((size_t)size);
    avcodec_fill_audio_frame(pFrame, pCodecCtx->channels, pCodecCtx->sample_fmt, frame_buf, size, 1);

    avformat_write_header(pFormatCtx, NULL);

    av_new_packet(&pkt, size);

    LOGD(DEBUG, "音频编码初始化完成")

    return 0;

}

void audio_encoder::encodeFrame(uint8* srcData) {
    pFrame->data[0] = srcData;
    // 该帧的时间
//    pFrame->pts = (utils::getCurrentTime() - arguments->start_time) / arguments->audio_sample_rate;
    pFrame->pts = frame_count ++;
    int got_frame = 0;
    int ret = avcodec_encode_audio2(pCodecCtx, &pkt, pFrame, &got_frame);
    if (ret < 0) {
        LOGE(DEBUG, "音频编码失败");
        return;
    }
    if (got_frame == 1) {
//        LOGD(DEBUG, "成功编码 1 帧音频! \t size:%5d\n", pkt.size);
//        pkt.pts = av_rescale_q_rnd(pFrame->pts,
//                                   audio_st->codec->time_base,
//                                   audio_st->time_base,
//                                   AV_ROUND_NEAR_INF);
        pkt.stream_index = audio_st->index;
        ret = av_write_frame(pFormatCtx, &pkt);
        av_free_packet(&pkt);
    }
}

void audio_encoder::stop() {
    LOGD(DEBUG, "flush_audio_encoder");
    int ret = flush_encoder(0);
    if (ret < 0) {
        LOGE(DEBUG, "flush encoder audio failed\n");
    }
    LOGD(DEBUG, "av_write_trailer");
    av_write_trailer(pFormatCtx);
    if (audio_st) {
        avcodec_close(audio_st->codec);
        av_free(pFrame);
        av_free(frame_buf);
    }
    avio_close(pFormatCtx->pb);
    avformat_free_context(pFormatCtx);
}

int audio_encoder::flush_encoder(unsigned int stream_index) {
    int ret;
    int got_frame;
    AVPacket enc_pkt;
    if (!(pFormatCtx->streams[stream_index]->codec->codec->capabilities & CODEC_CAP_DELAY)) {
        return 0;
    }
    while (1) {
        enc_pkt.data = NULL;
        enc_pkt.size = 0;
        av_init_packet(&enc_pkt);
        ret = avcodec_encode_audio2(pFormatCtx->streams[stream_index]->codec, &enc_pkt, NULL, &got_frame);
        av_frame_free(NULL);
        if (ret < 0) {
            break;
        }
        if (!got_frame) {
            break;
        }
        LOGD(DEBUG, "flush encoder: successed to encode 1 frame! \tsize:%5d\n", enc_pkt.size);
        ret = av_write_frame(pFormatCtx, &enc_pkt);
        if (ret < 0) {
            break;
        }
    }
    return ret;
}

