package com.pf.ffmpegmusicdemo;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;

/**
 * @author zhaopf
 * @version 1.0
 * @QQ 1308108803
 * @date 2017/9/25
 */

public class MusicPlayer {

    private AudioTrack audioTrack;

    static {
        System.loadLibrary("avcodec-56");
        System.loadLibrary("avdevice-56");
        System.loadLibrary("avfilter-5");
        System.loadLibrary("avformat-56");
        System.loadLibrary("avutil-54");
        System.loadLibrary("postproc-53");
        System.loadLibrary("swresample-1");
        System.loadLibrary("swscale-3");
        System.loadLibrary("native-lib");
    }

    /**
     * @param input  输入文件的路径
     * @param output 输出文件的路径
     */
    public native void changeFile(String input, String output);

    /**
     * @param input  输入文件的路径
     */
    public native void play(String input);

    /**
     * 这个方法是通过c调用的，不是给java调用的
     *
     * @param sampleRateInHz 音频采样率
     * @param nb_channels    声道数量
     */
    public void createAudio(int sampleRateInHz, int nb_channels) {
        int channelConfig;
        if (nb_channels == 2) {
            channelConfig = AudioFormat.CHANNEL_OUT_STEREO;
        } else {
            channelConfig = AudioFormat.CHANNEL_OUT_MONO;
        }
        int bufferSize = AudioTrack.getMinBufferSize(sampleRateInHz, channelConfig,
                AudioFormat.ENCODING_PCM_16BIT);
        audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, sampleRateInHz, channelConfig,
                AudioFormat.ENCODING_PCM_16BIT, bufferSize, AudioTrack.MODE_STREAM);
        audioTrack.play();
    }

    /**
     * 这个方法是通过c调用的，不是给java调用的
     *
     * @param buffer 音频数据
     * @param length 音频数据数组长度
     */
    public void playTrack(byte[] buffer, int length) {
        if (audioTrack != null && audioTrack.getPlayState() == AudioTrack.PLAYSTATE_PLAYING) {
            audioTrack.write(buffer, 0, length);
        }
    }
}