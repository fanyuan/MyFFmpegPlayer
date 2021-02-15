package com.example.myffmpeg

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.os.Environment
import android.view.View
import java.io.File

class AudioActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_audio)
    }
    fun decode(v: View){
        var basePath = Environment.getExternalStorageDirectory().absolutePath + File.separator + "tmp" + File.separator;
        var inputPath = basePath + "in.mp3"
        var outputPath = basePath + "out.pcm"
        AudioDecodeUtil.decode(inputPath,outputPath);
    }

}
