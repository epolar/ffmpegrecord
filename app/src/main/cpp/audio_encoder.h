//
// Created by eraise on 2017/11/2.
//

#ifndef LIBYUV_AUDIO_ENCODER_H
#define LIBYUV_AUDIO_ENCODER_H

#include "base_include.h"
#include "threadsafe_queue.cpp"
#include "unistd.h"

#ifdef __cplusplus
extern "C" {
#endif

struct audio_pcm_data {
    long len = NULL;
    long pts = NULL;
    uint8 *data = NULL;

    audio_pcm_data(long len, long pts, uint8 *srcData);
    void destory();
};

class audio_encoder {
public:
    int init(Arguments *vargs);

    void push_frame(uint8 *srcData, int len);

    void stop();

    int get_frame_size();

private:
    void encode_frame(audio_pcm_data *srcData);

    int flush_encoder(unsigned int stream_index);

    static void *start_encode(void *obj);

public:
    pthread_t encode_thread;
private:
    AVFormatContext *pFormatCtx;
    AVOutputFormat *fmt;
    AVStream *audio_st;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    AVPacket pkt;
    AVFrame *pFrame;
    threadsafe_queue<audio_pcm_data> queue;

    int size;
//    uint8_t* frame_buf;
    int64_t frame_count;
    bool isEnd;
    long start_time;

    Arguments *arguments;
    bool processing = false;
};

#endif //LIBYUV_AUDIO_ENCODER_H

#ifdef __cplusplus
}
#endif