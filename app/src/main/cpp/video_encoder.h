//
// Created by eraise on 2017/10/30.
//

#ifndef LIBYUV_VIDEO_ENCODER_H
#define LIBYUV_VIDEO_ENCODER_H

#include <base_include.h>
#include <threadsafe_queue.cpp>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavutil/opt.h>

#ifdef __cplusplus
}
#endif

class video_encoder {
public:
    int init(Arguments* vargs);
    void pushFrame(uint8 *srcData);
    void stop();
private:
    int flush_encoder(unsigned int stream_index);
    void encode_frame(uint8 *srcData);
    uint8 *frame_filter(uint8 *srcData);
    static void *start_encode(void *obj);

public:
    pthread_t encode_thread;
private:
    AVFormatContext* pFormatCtx;
    AVOutputFormat* fmt;
    AVStream* video_st;
    AVCodecContext* pCodecCtx;
    AVCodec* pCodec;
    AVPacket pkt;
    uint8_t* picture_buf;
    AVFrame* pFrame;

    threadsafe_queue<uint8 *> queue;

    int64_t frame_count;
    bool processing;
    bool isEnd = false;

    long last_encode_time;
    int frame_duration;

    Arguments* arguments;
};


#endif //LIBYUV_VIDEO_ENCODER_H
