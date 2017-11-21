#include "base_include.h"
#include "video_encoder.h"
#include "audio_encoder.h"
#include "media_muxer.h"

#ifndef _Included_xyz_eraise_libyuv_utils_NdkBridge
#define _Included_xyz_eraise_libyuv_utils_NdkBridge

#ifdef __cplusplus
extern "C" {
#endif

Arguments* arguments;
video_encoder* videoEncoder;
audio_encoder* audioEncoder;

JNIEXPORT int JNICALL Java_xyz_eraise_libyuv_utils_NdkBridge_prepare
        (JNIEnv *env,
         jclass jcls,
         jint in_width,
         jint in_height,
         jint out_width,
         jint out_height,
         jint rotatemodel,
         jboolean enable_mirror,
         jstring base_url,
         jstring video_name,
         jstring audio_name) {
    LOGI(DEBUG, "prepare 开始");
    arguments = new Arguments();
    arguments->in_width = in_width;
    arguments->in_height = in_height;
    arguments->out_width = out_width;
    arguments->out_height = out_height;
    arguments->rotate_model = rotatemodel;
    arguments->enable_mirror = enable_mirror;
    arguments->base_url = (char *) env->GetStringUTFChars(base_url, 0);
    arguments->video_name = (char *) env->GetStringUTFChars(video_name, 0);
    arguments->audio_name = (char *) env->GetStringUTFChars(audio_name, 0);
    arguments->start_time = utils::getCurrentTime();

    audioEncoder = new audio_encoder();
    audioEncoder->init(arguments);
    videoEncoder = new video_encoder();
    videoEncoder->init(arguments);

    LOGD(DEBUG, "初始化完成");

    return 0;
}

/**
 * 处理每一帧yuv：
 * 进行缩放、旋转、裁剪等操作后，编码到 h264
 */
JNIEXPORT void JNICALL Java_xyz_eraise_libyuv_utils_NdkBridge_processVideo
        (JNIEnv *env,
         jclass jcls,
         jbyteArray data) {
//    LOGD(DEBUG, "process...");

    jbyte* srcData = env->GetByteArrayElements(data, 0);

    videoEncoder->pushFrame((uint8 *) srcData);

    env->ReleaseByteArrayElements(data, srcData, 0);

}

/**
 * 处理音频，进行编码到aac
 */
JNIEXPORT void JNICALL Java_xyz_eraise_libyuv_utils_NdkBridge_processAudio
        (JNIEnv *env,
         jclass jcls,
         jbyteArray data) {
    jbyte* srcData = env->GetByteArrayElements(data, 0);

    audioEncoder->encodeFrame((uint8*) srcData);

    env->ReleaseByteArrayElements(data, srcData, 0);
}

JNIEXPORT void JNICALL Java_xyz_eraise_libyuv_utils_NdkBridge_stop
        (JNIEnv *env,
         jclass jcls) {
    LOGD(DEBUG, "stop...");
    videoEncoder->stop();
    audioEncoder->stop();
    pthread_join(videoEncoder->encode_thread, NULL);
    media_muxer::mux(arguments);
    LOGI(DEBUG, "Success !!!!!!!!!!!!!!!!!!!!");
}

#ifdef __cplusplus
}
#endif
#endif
