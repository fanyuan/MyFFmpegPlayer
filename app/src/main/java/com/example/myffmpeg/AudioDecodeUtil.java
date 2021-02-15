package com.example.myffmpeg;

public class AudioDecodeUtil {
    static {
        System.loadLibrary("player");
    }

    //    input.mp3    out.pcm
    public static native void decode(String input,String output);
}
