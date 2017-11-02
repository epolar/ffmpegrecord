//
// Created by eraise on 2017/10/30.
//

#include <base_include.h>

void logcallback(void* ptr, int level, const char* fmt,va_list vl){
    if (!fmt) {
        return;
    }
    static int print_prefix = 0;
    static int count;
    static char prev[1024];
    char line[1024];
    static int is_atty;

    // ffmpeg的日志回调得到的是： fmt为格式，vl为数据列表，需要进行格式化得到字符串以后再传给logcat
    av_log_format_line(ptr, level, fmt, vl, line, sizeof(line), &print_prefix);

    strcpy(prev, line);
    switch (level) {
        case AV_LOG_ERROR:
            LOGE(DEBUG, "%s", line);
        case AV_LOG_FATAL:
            LOGF(DEBUG, "%s", line);
            break;
        case AV_LOG_PANIC:
        case AV_LOG_WARNING:
            LOGW(DEBUG, "%s", line);
            break;
        case AV_LOG_DEBUG:
            LOGD(DEBUG, "%s", line);
            break;
        case AV_LOG_VERBOSE:
        case AV_LOG_INFO:
            LOGI(DEBUG, "%s", line);
            break;
    }
}