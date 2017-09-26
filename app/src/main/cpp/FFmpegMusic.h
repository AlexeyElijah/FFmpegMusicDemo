//
// Created by zhaopf on 2017/9/26.
//

#ifndef FFMPEGMUSICDEMO_FFMPEGMUSIC_H
#define FFMPEGMUSICDEMO_FFMPEGMUSIC_H

#endif //FFMPEGMUSICDEMO_FFMPEGMUSIC_H

extern "C"
{
//编码
#include "libavcodec/avcodec.h"
//封装格式处理
#include "libavformat/avformat.h"
//缩放处理
#include "libswscale/swscale.h"
//重采样
#include "libswresample/swresample.h"

#include <android/native_window_jni.h>
#include <unistd.h>
#include <android/log.h>
}

#define LOG_TAG "pf"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

/**
 *
 * @param input     输入的文件的路径
 * @param rate      采样率
 * @param channel   声道
 * @return          -1表示失败
 */
int createFFmpeg(char *input, int *rate, int *channel);

void getPCM(void **pcm, size_t *size);

/**
 * 释放资源
 */
void release();