package com.pf.ffmpegmusicdemo;

import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Toast;

import java.io.File;

public class MainActivity extends AppCompatActivity {

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
        MusicPlayer musicPlayer = new MusicPlayer();
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
        musicPlayer.changeFile(input, output);
    }

    /**
     * AudioTrack播放
     *
     * @param view
     */
    public void play(View view) {
        MusicPlayer musicPlayer = new MusicPlayer();
        File inputFile = new File(Environment.getExternalStorageDirectory(), "input.mp3");
        if (!inputFile.exists()) {
            Toast.makeText(this, "文件不存在", Toast.LENGTH_SHORT).show();
            return;
        }
        String input = inputFile.getAbsolutePath();
        musicPlayer.play(input);
    }

    /**
     * openSL播放
     *
     * @param view
     */
    public void sound(View view) {
        MusicPlayer musicPlayer = new MusicPlayer();
        musicPlayer.sound("/sdcard/input.mp3");
    }
}