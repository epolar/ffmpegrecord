//
// Created by eraise on 2017/10/30.
//

#ifndef LIBYUV_VIDEO_ENCODER_H
#define LIBYUV_VIDEO_ENCODER_H

#include <base_include.h>

class video_encoder {
public:
    int init(Arguments* vargs);
    void encodeFrame(uint8* srcData);
    void stop();
private:
    int flush_encoder(unsigned int stream_index);
private:
    AVFormatContext* pFormatCtx;
    AVOutputFormat* fmt;
    AVStream* video_st;
    AVCodecContext* pCodecCtx;
    AVCodec* pCodec;
    AVPacket pkt;
    uint8_t* picture_buf;
    AVFrame* pFrame;

    Arguments* arguments;
    u_long frame_count;
    bool processing = false;
};


#endif //LIBYUV_VIDEO_ENCODER_H
