# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
             yuvutil

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
              src/main/cpp/log.cpp
              src/main/cpp/video_encoder.cpp
              src/main/cpp/audio_encoder.cpp
              src/main/cpp/utils.cpp
              src/main/cpp/media_muxer.cpp
              src/main/cpp/threadsafe_queue.cpp
              src/main/cpp/Arguments.cpp
              src/main/cpp/xyz_eraise_libyuv_utils_NdkBridge.cpp
             )

add_library(libyuv
            SHARED
            IMPORTED)

add_library(
            avcodec
            SHARED
            IMPORTED
            )

add_library(
            avfilter
            SHARED
            IMPORTED
             )


add_library(
            avformat
            SHARED
            IMPORTED
            )


add_library(
            avutil
            SHARED
            IMPORTED
            )

add_library(
            swresample
            SHARED
            IMPORTED
            )

add_library(
            swscale
            SHARED
            IMPORTED
            )


add_library(
            fdk-aac
            SHARED
            IMPORTED
            )

set_target_properties(
    libyuv
    PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/libyuv_static.a
    )
set_target_properties(
    avcodec
    PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/libavcodec.so
    )

set_target_properties(
    avfilter
    PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/libavfilter.so
    )

set_target_properties(
    avformat
    PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/libavformat.so
    )

set_target_properties(
    avutil
    PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/libavutil.so
    )

set_target_properties(
    swresample
    PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/libswresample.so
     )

set_target_properties(
    swscale
    PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/libswscale.so
     )


set_target_properties(
    fdk-aac
    PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/libfdk-aac.so
     )

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

include_directories(src/main/cpp/include)

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       yuvutil
                       fdk-aac
                       avcodec
                       avfilter
                       avformat
                       avutil
                       swresample
                       swscale
                       libyuv
                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib} )