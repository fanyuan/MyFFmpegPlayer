package com.example.myffmpeg

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import androidx.databinding.DataBindingUtil
import com.example.myffmpeg.databinding.ActivityVideoBinding
import android.icu.lang.UCharacter.GraphemeClusterBreak.T
import androidx.core.content.ContextCompat.getSystemService
import android.R.attr.start
import android.os.Environment.getExternalStorageDirectory
import androidx.core.app.ComponentActivity
import androidx.core.app.ComponentActivity.ExtraData
import android.icu.lang.UCharacter.GraphemeClusterBreak.T
import android.os.Environment
import android.view.View
import androidx.core.content.ContextCompat.getSystemService
import java.io.File


class VideoActivity : AppCompatActivity() {

    lateinit var binding:ActivityVideoBinding;
    lateinit var player:MyPlayer
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        //setContentView(R.layout.activity_video)
        binding = DataBindingUtil.setContentView<ActivityVideoBinding>(this,R.layout.activity_video)
        binding.act = this
        binding.tv.text = FfmpegVideoUtil.getTestString()

        player = MyPlayer()
        player.setSurfaceView(binding.surfaceView)
    }
    fun open(view: View) {
        var path = Environment.getExternalStorageDirectory().absolutePath+ File.separator + "tmp" + File.separator + "test.mp4";
        val file = File(path)
        player.start(file.getAbsolutePath())
    }
}
