//
// Created by eraise on 2017/11/2.
//
#include "audio_encoder.h"

void audio_pcm_data::destory() {
    if (data) {
        free(data);
        data = NULL;
    }
}

audio_pcm_data::audio_pcm_data(long len, long pts, uint8 *srcData) {
    this->len = len;
    this->pts = pts;

    data = (uint8 *) malloc((size_t)len);
    memcpy(data, srcData, (size_t)len);
}

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

    avformat_write_header(pFormatCtx, NULL);

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
//    audio_pcm_data data = audio_pcm_data{len, utils::getCurrentTime() - start_time, srcData};
    queue.push(audio_pcm_data{len, utils::getCurrentTime() - start_time, srcData});
    processing = false;
}

void audio_encoder::encode_frame(audio_pcm_data *pcm_data) {
    uint8 *srcData = pcm_data->data;
    avcodec_fill_audio_frame(pFrame,
                                   pCodecCtx->channels,
                                   pCodecCtx->sample_fmt,
                                   srcData, (int)pcm_data->len, 0);
//    pFrame->pts = frame_count ++;
    if (avcodec_send_frame(pCodecCtx, pFrame) != 0) {
        LOGD(DEBUG, "send frame failed")
    }

    while (avcodec_receive_packet(pCodecCtx, &pkt) >= 0) {
        av_write_frame(pFormatCtx, &pkt);
        av_packet_unref(&pkt);
    }

}

void audio_encoder::stop() {
    isEnd = true;
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
            frame_buf.destory();
//            free(frame_buf.data);
//            free(&frame_buf);
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
