#include <jni.h>
#include <string>
#include <android/log.h>
#include "FFmpegMusic.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <assert.h>

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
}

#define LOG_TAG "pf"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

//创建audio结构体
SLObjectItf engineObject = NULL;
//音频引擎
SLEngineItf engineEngine;
//混音器接口
SLEnvironmentalReverbItf outputMixEnvironmentReverb = NULL;
//混音器
SLObjectItf outputMixObject = NULL;

const SLEnvironmentalReverbSettings settings = SL_I3DL2_ENVIRONMENT_PRESET_DEFAULT;
//缓冲队列播放器接口
SLObjectItf bgPlayerObject = NULL;
//播放接口
SLPlayItf bgPlayerPlay;
//缓冲器队列接口
SLAndroidSimpleBufferQueueItf bgPlayerBufferQueue;
//音量
SLVolumeItf bgPlayerVolume;
size_t bufferSize;
void *buffer;

// 当喇叭播放完声音时回调此方法
void bgPlayerCallback(SLAndroidSimpleBufferQueueItf bg, void *context) {
    bufferSize = 0;
    //assert(NULL == context);
    getPCM(&buffer, &bufferSize);
    // for streaming playback, replace this test by logic to find and fill the next buffer
    if (NULL != buffer && 0 != bufferSize) {
        SLresult result;
        // enqueue another buffer
        result = (*bgPlayerBufferQueue)->Enqueue(bgPlayerBufferQueue, buffer, bufferSize);
        // the most likely other result is SL_RESULT_BUFFER_INSUFFICIENT,
        // which for this code example would indicate a programming error
        assert(SL_RESULT_SUCCESS == result);
        LOGE("pf bgPlayerCallback :%d", result);
    }
}

// shut down the native audio system
void shutdown() {
    // destroy buffer queue audio player object, and invalidate all associated interfaces
    if (bgPlayerObject != NULL) {
        (*bgPlayerObject)->Destroy(bgPlayerObject);
        bgPlayerObject = NULL;
        bgPlayerPlay = NULL;
        bgPlayerBufferQueue = NULL;
        bgPlayerVolume = NULL;
    }

    // destroy output mix object, and invalidate all associated interfaces
    if (outputMixObject != NULL) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = NULL;
        outputMixEnvironmentReverb = NULL;
    }
    // destroy engine object, and invalidate all associated interfaces
    if (engineObject != NULL) {
        (*engineObject)->Destroy(engineObject);
        engineObject = NULL;
        engineEngine = NULL;
    }
    // 释放FFmpeg解码器相关资源
    release();
}

extern "C"
{
/**
 * 转成pcm文件
 */
JNIEXPORT void JNICALL
Java_com_pf_ffmpegmusicdemo_MusicPlayer_changeFile(JNIEnv *env, jobject instance, jstring input_,
                                                   jstring output_) {
    const char *input = env->GetStringUTFChars(input_, 0);
    const char *output = env->GetStringUTFChars(output_, 0);

    av_register_all();
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    //打开音频文件
    if (avformat_open_input(&pFormatCtx, input, NULL, NULL) != 0) {
        LOGE("音频文件打开失败");
        return;
    }
    //获取文件输入信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGE("获取文件信息失败");
        return;
    }
    //获取音频流索引位置
    int i = 0, audio_stream_idx = -1;
    for (; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_idx = i;
            break;
        }
    }
    //获取解码器
    AVCodecContext *pCodecCtx = pFormatCtx->streams[audio_stream_idx]->codec;
    AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    //打开解码器
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        LOGE("打开解码器失败");
        return;
    }
    //压缩数据
    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
    //解压缩数据
    AVFrame *frame = av_frame_alloc();
    //frame->16bit 44100 PCM 统一音频采样格式与采样率
    SwrContext *swrContext = swr_alloc();
    //音频格式,重采样设置参数
    AVSampleFormat in_sample_fmt = pCodecCtx->sample_fmt;
    //输出采样格式
    AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    //输入采样率
    int in_sample_rate = pCodecCtx->sample_rate;
    //输出采样率
    int out_sample_rate = 44100;
    //输入声道布局
    uint64_t in_ch_layout = pCodecCtx->channel_layout;
    //输出声道布局
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
    //设置参数
    swr_alloc_set_opts(swrContext, out_ch_layout, out_sample_fmt, out_sample_rate, in_ch_layout,
                       in_sample_fmt, in_sample_rate, 0, NULL);
    //初始化
    swr_init(swrContext);
    int got_frame = 0;
    int ret;
    int out_channel_nb = av_get_channel_layout_nb_channels(out_ch_layout);
    LOGE("声道数量:%d", out_channel_nb);
    int count = 0;
    //设置音频缓冲区间 16bit 44100  PCM数据
    uint8_t *out_buffer = (uint8_t *) av_malloc(2 * 44100);
    //打开文件
    FILE *fp_pcm = fopen(output, "wb");
    while (av_read_frame(pFormatCtx, packet) >= 0) {
        ret = avcodec_decode_audio4(pCodecCtx, frame, &got_frame, packet);
        LOGE("正在解码:%d", count++);
        if (ret < 0) {
            LOGE("解码完成");
        }
        //解码一帧
        if (got_frame > 0) {
            swr_convert(swrContext, &out_buffer, 2 * 44100, (const uint8_t **) frame->data,
                        frame->nb_samples);
            int out_buffer_size = av_samples_get_buffer_size(NULL, out_channel_nb,
                                                             frame->nb_samples, out_sample_fmt, 1);
            fwrite(out_buffer, 1, out_buffer_size, fp_pcm);
        }
    }
    //释放资源
    fclose(fp_pcm);
    av_frame_free(&frame);
    av_free(out_buffer);
    swr_free(&swrContext);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
    env->ReleaseStringUTFChars(input_, input);
    env->ReleaseStringUTFChars(output_, output);
}

/**
 * AudioTrack播放
 */
JNIEXPORT void JNICALL
Java_com_pf_ffmpegmusicdemo_MusicPlayer_play(JNIEnv *env, jobject instance, jstring input_) {
    const char *input = env->GetStringUTFChars(input_, 0);

    av_register_all();
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    //打开音频文件
    if (avformat_open_input(&pFormatCtx, input, NULL, NULL) != 0) {
        LOGE("音频文件打开失败");
        return;
    }
    //获取文件输入信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGE("获取文件信息失败");
        return;
    }
    //获取音频流索引位置
    int i, audio_stream_idx = -1;
    for (i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_idx = i;
            break;
        }
    }
    //获取解码器
    AVCodecContext *pCodecCtx = pFormatCtx->streams[audio_stream_idx]->codec;
    AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    //打开解码器
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        LOGE("打开解码器失败");
        return;
    }
    //压缩数据
    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
    //解压缩数据
    AVFrame *frame = av_frame_alloc();
    //frame->16bit 44100 PCM 统一音频采样格式与采样率
    SwrContext *swrContext = swr_alloc();
    //音频格式,重采样设置参数
    AVSampleFormat in_sample_fmt = pCodecCtx->sample_fmt;
    //输出采样格式
    AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    //输入采样率
    int in_sample_rate = pCodecCtx->sample_rate;
    //输出采样率
    int out_sample_rate = pCodecCtx->sample_rate;
    //输入声道布局
    uint64_t in_ch_layout = pCodecCtx->channel_layout;
    //输出声道布局
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
    //设置参数
    swr_alloc_set_opts(swrContext, out_ch_layout, out_sample_fmt, out_sample_rate, in_ch_layout,
                       in_sample_fmt, in_sample_rate, 0, NULL);
    //初始化
    swr_init(swrContext);
    int got_frame = 0;
    int ret;
    int out_channel_nb = av_get_channel_layout_nb_channels(out_ch_layout);
    LOGE("声道数量:%d", out_channel_nb);

    //设置音频缓冲区间 16bit 44100  PCM数据
    uint8_t *out_buffer = (uint8_t *) av_malloc(2 * 44100);

    //获取class
    jclass music_player = env->GetObjectClass(instance);
    //得到createAudio方法
    jmethodID createAudioID = env->GetMethodID(music_player, "createAudio", "(II)V");
    //调用createAudio
    env->CallVoidMethod(instance, createAudioID, 44100, out_channel_nb);
    jmethodID playTrackID = env->GetMethodID(music_player, "playTrack", "([BI)V");

    int count = 0;
    while (av_read_frame(pFormatCtx, packet) >= 0) {
        if (packet->stream_index == audio_stream_idx) {
            //解码
            avcodec_decode_audio4(pCodecCtx, frame, &got_frame, packet);
            if (got_frame) {
                LOGE("解码:%d", count++);
                swr_convert(swrContext, &out_buffer, 44100 * 2, (const uint8_t **) frame->data,
                            frame->nb_samples);
                //缓冲区的大小
                int size = av_samples_get_buffer_size(NULL, out_channel_nb, frame->nb_samples,
                                                      AV_SAMPLE_FMT_S16P, 1);
                jbyteArray audio_sample_array = env->NewByteArray(size);
                env->SetByteArrayRegion(audio_sample_array, 0, size, (const jbyte *) out_buffer);
                env->CallVoidMethod(instance, playTrackID, audio_sample_array, size);
                env->DeleteLocalRef(audio_sample_array);
            }
        }
    }
    av_frame_free(&frame);
    swr_free(&swrContext);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);

    env->ReleaseStringUTFChars(input_, input);
}

/**
 * openSL播放
 */
JNIEXPORT void JNICALL
Java_com_pf_ffmpegmusicdemo_MusicPlayer_sound(JNIEnv *env, jobject instance, jstring input_) {
    const char *input = env->GetStringUTFChars(input_, 0);
    char *inputStr = (char *) input;
    SLresult sLresult;
    slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);

    (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    //获取SLEngine对象,后续的操作将使用这个对象
    (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    //用音频引擎调用函数,创建混音器outputMixObject
    (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 0, 0, 0);
    //实现混音器
    (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    //获取混音器接口
    (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                     &outputMixEnvironmentReverb);
    if (SL_RESULT_SUCCESS == sLresult) {
        (*outputMixEnvironmentReverb)->SetEnvironmentalReverbProperties(
                outputMixEnvironmentReverb, &settings);
    }
    int rate, channels;
    createFFmpeg(inputStr, &rate, &channels);
    LOGE("比特率:%d,channels:%d", rate, channels);

    //配置信息设置
    SLDataLocator_AndroidSimpleBufferQueue android_queue =
            {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    //参数:pcm,通道数,采样率,采样位数,包含位数,声道,end标志位
    SLDataFormat_PCM pcm = {SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_44_1,
                            SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
                            SL_BYTEORDER_LITTLEENDIAN};
    SLDataSource slDataSource = {&android_queue, &pcm};
    //设置混音器
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink pAudioSnk = {&outputMix, NULL};
    //创建Recorder需要 RECORD_AUDIO 权限
//    SLInterfaceID slInterfaceID[2]={SL_IID_ANDROIDSIMPLEBUFFERQUEUE,SL_IID_ANDROIDCONFIGURATION};
//    SLboolean reqs[2] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
    // create audio player
    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND,
            /*SL_IID_MUTESOLO,*/ SL_IID_VOLUME};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
            /*SL_BOOLEAN_TRUE,*/ SL_BOOLEAN_TRUE};
    int result = SL_RESULT_SUCCESS == sLresult;
    sLresult = (*engineEngine)->CreateAudioPlayer(engineEngine, &bgPlayerObject, &slDataSource,
                                                  &pAudioSnk, 3, ids, req);
    LOGE("初始化播放器:%d,是否成功:%d,bgPlayerObject:%d", sLresult, result, bgPlayerObject);

    //初始化播放器
    (*bgPlayerObject)->Realize(bgPlayerObject, SL_BOOLEAN_FALSE);
    //得到接口后调用,获取player接口
    (*bgPlayerObject)->GetInterface(bgPlayerObject, SL_IID_PLAY, &bgPlayerPlay);
    //注册回调缓冲区,获取缓冲队列接口
    (*bgPlayerObject)->GetInterface(bgPlayerObject, SL_IID_BUFFERQUEUE, &bgPlayerBufferQueue);
    LOGE("获取缓冲区数据");
    //缓冲接口回调
    (*bgPlayerBufferQueue)->RegisterCallback(bgPlayerBufferQueue, bgPlayerCallback, NULL);
    //获取音量接口
    (*bgPlayerObject)->GetInterface(bgPlayerObject, SL_IID_VOLUME, &bgPlayerVolume);
    //获取播放状态接口
    (*bgPlayerPlay)->SetPlayState(bgPlayerPlay, SL_PLAYSTATE_PLAYING);

    bgPlayerCallback(bgPlayerBufferQueue, NULL);

    env->ReleaseStringUTFChars(input_, input);
}

/**
 * 对openSL播放停止播放
 */
JNIEXPORT void JNICALL
Java_com_pf_ffmpegmusicdemo_MusicPlayer_stop(JNIEnv *env, jobject instance) {
    shutdown();
}
}