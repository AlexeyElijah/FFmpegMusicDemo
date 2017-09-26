//
// Created by zhaopf on 2017/9/26.
//

#include "FFmpegMusic.h"

//上下文
AVFormatContext *pFormatCtx;
//音频流下标
int audio_stream_idx = -1;
//音频解码器
AVCodecContext *pCodecCtx;
//音频编码
AVCodec *pCodec;
//音频数据
AVFrame *frame;
//一帧的数据
AVPacket *packet;

struct SwrContext *swrContext;
uint8_t *out_buffer;

int createFFmpeg(char *input, int *rate, int *channel) {
    av_register_all();
    pFormatCtx = avformat_alloc_context();
    if (avformat_open_input(&pFormatCtx, input, NULL, NULL) < 0) {
        LOGE("打开输入视频文件失败");
        return -1;
    }
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGE("获取视频信息失败");
        return -1;
    }
    int i = 0;
    for (; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            LOGE("找到音频下标");
            audio_stream_idx = i;
            break;
        }
    }
    //获取音频解码器
    pCodecCtx = pFormatCtx->streams[audio_stream_idx]->codec;
    //获取音频编码
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        LOGE("打开失败");
        return -1;
    }
    //音频数据
    frame = av_frame_alloc();
    //一帧的数据
    packet = (AVPacket *) av_malloc(sizeof(AVPacket));

    swrContext = swr_alloc();
    swr_alloc_set_opts(swrContext, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16,
                       pCodecCtx->sample_rate, pCodecCtx->channel_layout, pCodecCtx->sample_fmt,
                       pCodecCtx->sample_rate, 0, 0);
    swr_init(swrContext);
    *rate = pCodecCtx->sample_rate;
    *channel = pCodecCtx->channels;
    out_buffer = (uint8_t *) av_malloc(44100 * 2);
    LOGE("ffmpeg初始化完毕");
    return 0;
}

void getPCM(void **pcm, size_t *size) {
    int got_frame;
    int out_channel_nb = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    while (av_read_frame(pFormatCtx, packet) >= 0) {
        if (packet->stream_index == audio_stream_idx) {
            avcodec_decode_audio4(pCodecCtx, frame, &got_frame, packet);
            if (got_frame) {
                swr_convert(swrContext, &out_buffer, 44100 * 2, (const uint8_t **) frame->data,
                            frame->nb_samples);
                //通道数
                int out_buffer_size = av_samples_get_buffer_size(NULL, out_channel_nb,
                                                                 frame->nb_samples,
                                                                 AV_SAMPLE_FMT_S16, 1);
                *pcm = out_buffer;
                *size = out_buffer_size;
                break;
            }
        }
    }
}


void release() {
    av_free_packet(packet);
    av_free(out_buffer);
    av_frame_free(&frame);
    swr_free(&swrContext);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
}