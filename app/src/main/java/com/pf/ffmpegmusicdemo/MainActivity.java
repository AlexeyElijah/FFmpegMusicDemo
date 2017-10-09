package com.pf.ffmpegmusicdemo;

import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Toast;

import java.io.File;

public class MainActivity extends AppCompatActivity {

    private MusicPlayer openSLMusicPlayer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
    }

    /**
     * 转换成pcm文件
     *
     * @param view
     */
    public void change(View view) {
        File inputFile = new File(Environment.getExternalStorageDirectory(), "input.mp3");
        if (!inputFile.exists()) {
            Toast.makeText(this, "文件不存在", Toast.LENGTH_SHORT).show();
            return;
        }
        String input = inputFile.getAbsolutePath();
        File outputFile = new File(Environment.getExternalStorageDirectory(), "output.pcm");
        if (outputFile.exists()) {
            outputFile.delete();
        }
        String output = outputFile.getAbsolutePath();
        MusicPlayer musicPlayer = new MusicPlayer();
        musicPlayer.changeFile(input, output);
    }

    /**
     * AudioTrack播放
     *
     * @param view
     */
    public void play(View view) {
        File inputFile = new File(Environment.getExternalStorageDirectory(), "input.mp3");
        if (!inputFile.exists()) {
            Toast.makeText(this, "文件不存在", Toast.LENGTH_SHORT).show();
            return;
        }
        String input = inputFile.getAbsolutePath();
        MusicPlayer musicPlayer = new MusicPlayer();
        musicPlayer.play(input);
    }

    /**
     * openSL播放
     *
     * @param view
     */
    public void sound(View view) {
        File inputFile = new File(Environment.getExternalStorageDirectory(), "input.mp3");
        if (!inputFile.exists()) {
            Toast.makeText(this, "文件不存在", Toast.LENGTH_SHORT).show();
            return;
        }
        openSLMusicPlayer = new MusicPlayer();
        openSLMusicPlayer.sound("/sdcard/input.mp3");
    }

    /**
     * 停止播放
     */
    public void stop(View view) {
        if (openSLMusicPlayer != null) {
            openSLMusicPlayer.stop();
        }
    }
}