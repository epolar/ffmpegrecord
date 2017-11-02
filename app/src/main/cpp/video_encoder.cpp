//
// Created by eraise on 2017/10/30.
//

#include <video_encoder.h>

int video_encoder::init(Arguments* vargs) {
    arguments = vargs;
    LOGI(DEBUG, "av_register_all");
    av_register_all();
    pFormatCtx = avformat_alloc_context();
    fmt = av_guess_format(NULL, arguments->video_name, NULL);
    LOGI(DEBUG, "av_guess_format %s", fmt->name);
    pFormatCtx->oformat = fmt;
    av_log_set_callback(logcallback);

    char* output_url = strcat(arguments->video_base_url, arguments->video_name);
    LOGI(DEBUG, "output_url == %s", output_url);

    // 打开写入文件
    if (avio_open(&pFormatCtx->pb, output_url, AVIO_FLAG_READ_WRITE) < 0) {
        LOGE(DEBUG, "写入文件打开失败");
        return -1;
    }

    // 打开视频输出流
    video_st = avformat_new_stream(pFormatCtx, 0);

    if (video_st == NULL) {
        LOGE(DEBUG, "视频输出流打开失败");
        return -1;
    }

    //Param that must set
    pCodecCtx = video_st->codec;
    // 设置编码参数
    pCodecCtx->codec_id = fmt->video_codec;
    pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    pCodecCtx->width = arguments->out_width;
    pCodecCtx->height = arguments->out_height;
    pCodecCtx->bit_rate = 400000;
    pCodecCtx->gop_size = 300;
    pCodecCtx->thread_count = 15;

    pCodecCtx->time_base.num = 1;
    // 帧率
    pCodecCtx->time_base.den = 30;

    pCodecCtx->qmin = 10;
    pCodecCtx->qmax = 51;

    pCodecCtx->max_b_frames = 3;

    // Set Option
    AVDictionary *param = 0;
    //H.264
    if(pCodecCtx->codec_id == AV_CODEC_ID_H264) {
        av_dict_set(&param, "preset", "slow", 0);
        av_dict_set(&param, "tune", "zerolatency", 0);
        //av_dict_set(&param, "profile", "main", 0);
    }
    //H.265
    if(pCodecCtx->codec_id == AV_CODEC_ID_H265){
        av_dict_set(&param, "preset", "ultrafast", 0);
        av_dict_set(&param, "tune", "zero-latency", 0);
    }

    // 显示视频的一些信息
    av_dump_format(pFormatCtx, 0, output_url, 1);

    // 查找对应 codec_id 的编码器
    pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
    if (!pCodec){
        LOGE(DEBUG, "Can not find encoder");
        return -1;
    }
    // 打开编码器
    if (avcodec_open2(pCodecCtx, pCodec, &param) < 0){
        LOGE(DEBUG, "Failed to open encoder!");
        return -1;
    }

    pFrame = av_frame_alloc();
    pFrame->width = pCodecCtx->width;
    pFrame->height = pCodecCtx->height;
    pFrame->format = pCodecCtx->pix_fmt;

    int picture_size = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
    picture_buf = (uint8_t *)av_malloc((size_t)picture_size * 3);
    avpicture_fill((AVPicture *)pFrame, picture_buf, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);

    avformat_write_header(pFormatCtx,NULL);

    av_new_packet(&pkt,picture_size);
    return 0;
}

void video_encoder::encodeFrame(uint8* srcData) {
    if (processing) {
        return;
    }
    processing = true;

    int in_width = arguments->in_width;
    int in_height = arguments->in_height;
    int out_width = arguments->out_width;
    int out_height = arguments->out_height;
    int rotate_model = arguments->rotate_model;
    bool enable_mirror = arguments->enable_mirror;

//    LOGD(DEBUG, "准备转换为i420");
    int srcYSize = in_width * in_height;
    int srcUSize = (in_width >> 1) * (in_height >> 1);
    int srcVSize = srcUSize;
    int srcSize = srcYSize + srcUSize + srcVSize;
    int srcStrideY = in_width;
    int srcStrideU = in_width >> 1;
    int srcStrideV = srcStrideU;
    uint8 *srcDataY = (uint8 *)srcData;
    uint8 *srcDataU = (uint8 *)srcData + srcYSize;
    uint8 *srcDataV = (uint8 *)srcData + srcYSize + srcUSize;
    // 转换成i420
    uint8 *tempUVData = (uint8 *)malloc((size_t)srcUSize);
    memcpy(tempUVData, srcDataU, (size_t)srcUSize);
    memcpy(srcDataU, srcDataV, (size_t)srcUSize);
    memcpy(srcDataV, tempUVData, (size_t)srcUSize);
    free(tempUVData);
//    LOGD(DEBUG, "转换为i420完成");
//    uint8 *i420Data = (uint8 *)malloc((size_t)srcSize);
//    uint8 *i420DataY = i420Data;
//    uint8 *i420DataU = i420DataY + srcYSize;
//    uint8 *i420DataV = i420DataY + srcUSize + srcYSize;
//    libyuv::ConvertToI420(srcData, (size_t)srcSize,
//                          i420DataY, srcStrideY,
//                          i420DataU, srcStrideU,
//                          i420DataV, srcStrideV,
//                          0, 0, in_width,
//                          in_height, in_width, in_height,
//                          libyuv::kRotate0, libyuv::FOURCC_YV12);

    // 等比缩放
//    LOGD(DEBUG, "准备等比缩放");
    float widthRatio, heightRatio;
    if (rotate_model == libyuv::kRotate90 ||
        rotate_model == libyuv::kRotate270) {
        widthRatio = (float)out_width / (float)in_height;
        heightRatio = float(out_height) / float(in_width);
    } else {
        widthRatio = out_width / in_width;
        heightRatio = out_height / in_height;
    }
    float scaleRatio = widthRatio < heightRatio ? heightRatio : widthRatio;
    int scaleWidth = (int)(scaleRatio * in_width);
    int scaleHeight = (int)(scaleRatio * in_height);
    int scaleYSize = scaleWidth * scaleHeight;
    int scaleUSize = (scaleWidth >> 1) * (scaleHeight >> 1);
    int scaleVSize = scaleUSize;
    int scaleSize = scaleYSize + scaleUSize + scaleVSize;
    int scaleStrideY = scaleWidth;
    int scaleStrideU = scaleWidth >> 1;
    int scaleStrideV = scaleStrideU;
    uint8 *scaleData = (uint8 *)malloc((size_t)scaleSize);
    uint8 *scaleDataY = scaleData;
    uint8 *scaleDataU = scaleData + scaleYSize;
    uint8 *scaleDataV = scaleData + scaleYSize + scaleUSize;
    libyuv::I420Scale(srcDataY, srcStrideY,
                      srcDataU, srcStrideU,
                      srcDataV, srcStrideV,
                      in_width, in_height,
                      scaleDataY, scaleStrideY,
                      scaleDataU, scaleStrideU,
                      scaleDataV, scaleStrideV,
                      scaleWidth, scaleHeight,
                      libyuv::FilterMode::kFilterBilinear);
//    LOGD(DEBUG, "等比缩放完成");

//    LOGD(DEBUG, "准备旋转&裁剪");
    // 旋转 & 裁剪
    int cropYSize = out_width * out_height;
    int cropUSize = (out_width >> 1) * (out_height >> 1);
    int cropVSize = cropUSize;
    int cropSize = cropYSize + cropUSize + cropVSize;
    int cropStrideY = out_width;
    int cropStrideU = out_width >> 1;
    int cropStrideV = cropStrideU;
    int cropWidth, cropHeight;
    if (rotate_model == 90 || rotate_model == 270) {
        cropWidth = out_height;
        cropHeight = out_width;
    } else {
        cropWidth = out_width;
        cropHeight = out_height;
    }
    uint8 *cropData = (uint8 *) malloc((size_t)cropSize);
    uint8 *cropDataY = cropData;
    uint8 *cropDataU = cropData + cropYSize;
    uint8 *cropDataV = cropData + cropYSize + cropUSize;
    libyuv::ConvertToI420(scaleData, (size_t)scaleSize,
                          cropDataY, cropStrideY,
                          cropDataU, cropStrideU,
                          cropDataV, cropStrideV,
                          0, 0,
                          scaleWidth, scaleHeight,
                          cropWidth, cropHeight,
                          (libyuv::RotationMode) rotate_model, libyuv::FOURCC_I420);
    free(scaleData);
//    LOGD(DEBUG, "旋转&裁剪完成");

    // 镜像
    if (enable_mirror) {
//        LOGD(DEBUG, "准备镜像");
        uint8 *mirrorData = (uint8 *) malloc((size_t)cropSize);
        uint8 *mirrorDataY = mirrorData;
        uint8 *mirrorDataU = mirrorData + cropYSize;
        uint8 *mirrorDataV = mirrorData + cropYSize + cropUSize;
        libyuv::I420Mirror(cropDataY, cropStrideY,
                           cropDataU, cropStrideU,
                           cropDataV, cropStrideV,
                           mirrorDataY, cropStrideY,
                           mirrorDataU, cropStrideU,
                           mirrorDataV, cropStrideV,
                           out_width, out_height);
        free(cropData);
        cropData = mirrorData;
        cropDataY = mirrorDataY;
        cropDataU = mirrorDataU;
        cropDataV = mirrorDataV;
//        LOGD(DEBUG, "镜像完成");
    }

//    LOGD(DEBUG, "设置data[0]");
    // 设置yuv数据到 AVFrame
    pFrame->data[0] = cropDataY;
//    LOGD(DEBUG, "设置data[1]");
    pFrame->data[1] = cropDataU;
//    LOGD(DEBUG, "设置data[2]");
    pFrame->data[2] = cropDataV;
    // 该yuv在视频的第几帧
    pFrame->pts = frame_count ++;
    int got_picture = 0;
    // 进行编码
//    LOGD(DEBUG, "avcodec_encode_video2");
    int ret = avcodec_encode_video2(pCodecCtx, &pkt, pFrame, &got_picture);
//    LOGD(DEBUG, "avcodec_encode_video2 成功");
    if(ret < 0){
        LOGE(DEBUG, "Failed to encode!");
    }
    if (got_picture==1){
//        LOGI(DEBUG, "Succeed to encode frame: %5ld\tsize:%5d\n",frame_count,pkt.size);
        pkt.stream_index = video_st->index;
        ret = av_write_frame(pFormatCtx, &pkt);
        av_packet_unref(&pkt);
    }
//    LOGD(DEBUG, "av_write_frame 处理完成");

//    LOGI("转换回YV12");
//    tempUVData = (uint8 *)malloc((size_t)cropSize);
//    memcpy(tempUVData, cropDataU, (size_t)cropUSize);
//    memcpy(cropDataU, cropDataV, (size_t)cropUSize);
//    memcpy(cropDataV, tempUVData, (size_t)cropUSize);
//    free(tempUVData);
//    uint8 *yvData = (uint8 *)malloc((size_t)cropSize);
//    libyuv::ConvertFromI420(cropDataY, cropStrideY,
//                            cropDataU, cropStrideU,
//                            cropDataV, cropStrideV,
//                            yvData, out_width,
//                            out_width, out_height,
//                            libyuv::FOURCC_YV12);
//    LOGI("结束I420 -> YV12");
//    free(cropData);

//    env->SetByteArrayRegion(resultdata, 0, cropSize, (jbyte *)cropData);
//    free(yvData);
    free(cropData);

    processing = false;
}

int video_encoder::flush_encoder(unsigned int stream_index) {
    LOGD(DEBUG, "flush_encoder");
    int ret;
    int got_frame;
    AVPacket enc_pkt;
    if (!(pFormatCtx->streams[stream_index]->codec->codec->capabilities &
          CODEC_CAP_DELAY))
        return 0;
    while (1) {
        enc_pkt.data = NULL;
        enc_pkt.size = 0;
        av_init_packet(&enc_pkt);
        ret = avcodec_encode_video2 (pFormatCtx->streams[stream_index]->codec, &enc_pkt,
                                     NULL, &got_frame);
        av_frame_free(NULL);
        if (ret < 0)
            break;
        if (!got_frame){
            ret=0;
            break;
        }
        LOGI(DEBUG, "Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n", enc_pkt.size);
        /* mux encoded frame */
        ret = av_write_frame(pFormatCtx, &enc_pkt);
        if (ret < 0)
            break;
    }
    return ret;
}

void video_encoder::stop() {
    while (processing) {
        // ... 阻塞着等 processing 为false
    }
    // 刷新缓存区数据
    int ret = flush_encoder(0);
    if (ret < 0) {
        LOGE(DEBUG, "Flushing encoder failed");
    }

    av_write_trailer(pFormatCtx);

    // 清空无用内存
    if (video_st){
        avcodec_close(video_st->codec);
        av_free(pFrame);
        av_free(picture_buf);
    }
    avio_close(pFormatCtx->pb);
    avformat_free_context(pFormatCtx);
}