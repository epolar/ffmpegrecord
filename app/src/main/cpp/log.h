//
// Created by eraise on 2017/9/21.
//
#include "android/log.h"

#define LOG_TAG "来自JNI:"

#define LOGE(debug, ...) if(debug){__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__);} // 定义LOGE类型
#define LOGD(debug, ...) if(debug){__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__);} // 定义LOGD类型
#define LOGI(debug, ...) if(debug){__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__);}  // 定义LOGI类型
#define LOGW(debug, ...) if(debug){__android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__);}  // 定义LOGW类型
#define LOGF(debug, ...) if(debug){__android_log_print(ANDROID_LOG_FATAL, LOG_TAG, __VA_ARGS__);} // 定义LOGF类型

const bool DEBUG = true;

void logcallback(void* ptr, int level, const char* fmt,va_list vl);