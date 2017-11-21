# This is the Android makefile for libyuv for NDK.
LOCAL_PATH:= $(call my-dir)

# common_CFLAGS := -Wall -fexceptions

LIB_DIR := ../jniLibs/$(TARGET_ARCH_ABI)

INCLUDE_PATH := $(LOCAL_PATH)/include

include $(CLEAR_VARS)
LOCAL_MODULE := avcodec
LOCAL_SRC_FILES := $(LIB_DIR)/libavcodec.so
LOCAL_EXPORT_C_INCLUDES := $(INCLUDE_PATH)
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := avfilter
LOCAL_SRC_FILES := $(LIB_DIR)/libavfilter.so
LOCAL_EXPORT_C_INCLUDES := $(INCLUDE_PATH)
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := avformat
LOCAL_SRC_FILES := $(LIB_DIR)/libavformat.so
LOCAL_EXPORT_C_INCLUDES := $(INCLUDE_PATH)
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := avutil
LOCAL_SRC_FILES := $(LIB_DIR)/libavutil.so
LOCAL_EXPORT_C_INCLUDES := $(INCLUDE_PATH)
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := fdk-aac
LOCAL_SRC_FILES := $(LIB_DIR)/libfdk-aac.so
LOCAL_EXPORT_C_INCLUDES := $(INCLUDE_PATH)
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := postproc
LOCAL_SRC_FILES := $(LIB_DIR)/libpostproc.so
LOCAL_EXPORT_C_INCLUDES := $(INCLUDE_PATH)
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := swresample
LOCAL_SRC_FILES := $(LIB_DIR)/libswresample.so
LOCAL_EXPORT_C_INCLUDES := $(INCLUDE_PATH)
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := swscale
LOCAL_SRC_FILES := $(LIB_DIR)/libswscale.so
LOCAL_EXPORT_C_INCLUDES := $(INCLUDE_PATH)
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := yuv
LOCAL_SRC_FILES := $(LIB_DIR)/libyuv_static.a
LOCAL_EXPORT_C_INCLUDES := $(INCLUDE_PATH)
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog -lz
LOCAL_MODULE := yuvutil
LOCAL_SRC_FILES := xyz_eraise_libyuv_utils_NdkBridge.cpp \
                   Arguments.cpp \
                   log.cpp \
                   video_encoder.cpp \
                   audio_encoder.cpp \
                   utils.cpp \
                   media_muxer.cpp \
                   threadsafe_queue.cpp
LOCAL_C_INCLUDES += -L$(SYSROOT)/usr/include
LOCAL_C_INCLUDES += $(INCLUDE_PATH)
LOCAL_SHARED_LIBRARIES := fdk-aac \
                          avcodec \
                          avfilter \
                          avformat \
                          avutil \
                          swresample \
                          swscale \
                          yuv
include $(BUILD_SHARED_LIBRARY)