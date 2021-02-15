//
// Created by develop on 2021/2/15.
//

#ifndef MYFFMPEGVIDEO_MYFFMPEG_H
#define MYFFMPEGVIDEO_MYFFMPEG_H

#include "JavaCallHelper.h"
#include "AudioChannel.h"
#include "VideoChannel.h"
#include "macro.h"
#include <cstring>
#include <pthread.h>

extern "C" {
#include <libavformat/avformat.h>
};

class MyFFmpeg {
    friend void *task_stop(void *args);
public:
    MyFFmpeg(JavaCallHelper *javaCallHelper, char *dataSource);

    ~MyFFmpeg();

    void prepare();

    void _prepare();

    void start();

    void _start();

    void setRenderCallback(RenderCallback renderCallback);

    void stop();

    void pause();

    void resume();

private:
    JavaCallHelper *javaCallHelper = 0;
    AudioChannel *audioChannel = 0;
    VideoChannel *videoChannel = 0;
    char *dataSource;
    pthread_t pid_prepare;
    pthread_t pid_start;
    pthread_t pid_stop;
    bool isPlaying;
    bool isPause;
    AVFormatContext *formatContext = 0;
    RenderCallback renderCallback;
    int duration;
    pthread_mutex_t seekMutex;
public:
    void setDuration(int duration);

    int getDuration() const;
//总播放时长

    void seekTo(int i);
};


#endif //MYFFMPEGVIDEO_MYFFMPEG_H