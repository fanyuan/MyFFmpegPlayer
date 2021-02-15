package com.example.myffmpeg;

public class FfmpegVideoUtil {
    static {
        System.loadLibrary("player");
    }
    public static native String getTestString();
}
