//
// Created by eraise on 2017/11/2.
//

#ifndef LIBYUV_AUDIO_ENCODER_H
#define LIBYUV_AUDIO_ENCODER_H

#include <base_include.h>

class audio_encoder {
public:
    int init(Arguments* vargs);
    void encodeFrame(uint8* srcData);
    void stop();
private:
    int flush_encoder(unsigned int stream_index);
private:
    AVFormatContext* pFormatCtx;
    AVOutputFormat* fmt;
    AVStream* audio_st;
    AVCodecContext* pCodecCtx;
    AVCodec* pCodec;
    AVPacket pkt;
    AVFrame* pFrame;

    u_long frame_count;
    int size;
    uint8_t* frame_buf;

    Arguments* arguments;
    bool processing = false;
};


#endif //LIBYUV_VIDEO_ENCODER_H
