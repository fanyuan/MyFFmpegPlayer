//
// Created by develop on 2021/2/15.
//

#include <jni.h>
#include <string>
#include "MyFFmpeg.h"
#include <android/native_window_jni.h>

extern "C" {
#include "include/libavutil/avutil.h"
}
/**
 * 整个c代码的入口
 */

JavaVM *javaVM = 0;
JavaCallHelper *javaCallHelper = 0;
MyFFmpeg *ffmpeg = 0;
ANativeWindow *window = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;//静态初始化mutex

jint JNI_OnLoad(JavaVM *vm,void *reserved){
    javaVM = vm;
    //TODO 注意点
    return JNI_VERSION_1_4;
}
//1，data;2，linesize；3，width; 4， height
void renderFrame(uint8_t *src_data, int src_lineSize, int width, int height) {
    pthread_mutex_lock(&mutex);

    if(!window){
        pthread_mutex_unlock(&mutex);
        return;
    }
    ANativeWindow_setBuffersGeometry(window,width,height,WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer window_buffer;
    if(ANativeWindow_lock(window,&window_buffer,0)){
        ANativeWindow_release(window);
        window = 0;
        pthread_mutex_unlock(&mutex);
        return;
    }
    //把buffer中的数据进行赋值（修改）
    uint8_t *dst_data = static_cast<uint8_t *>(window_buffer.bits);
    int dst_lineSize = window_buffer.stride * 4;//ARGB
    //遂行制拷贝
    for(int i = 0; i < window_buffer.height;++i){
        memcpy(dst_data + i * dst_lineSize, src_data + i * src_lineSize,dst_lineSize);
    }
    ANativeWindow_unlockAndPost(window);

    pthread_mutex_unlock(&mutex);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_myffmpeg_MyFfmpegPlayer_prepareNative(JNIEnv *env, jobject thiz,jstring jdata_source) {
    const char *dataSource = env->GetStringUTFChars(jdata_source,JNI_FALSE);

    javaCallHelper = new JavaCallHelper(javaVM,env,thiz);
    ffmpeg = new MyFFmpeg(javaCallHelper, const_cast<char *>(dataSource));
    ffmpeg->setRenderCallback(renderFrame);
    ffmpeg->prepare();

    env->ReleaseStringUTFChars(jdata_source,dataSource);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_myffmpeg_MyFfmpegPlayer_startNative(JNIEnv *env, jobject thiz) {
    if(ffmpeg){
        ffmpeg->start();
    }
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_myffmpeg_MyFfmpegPlayer_setSurfaceNative(JNIEnv *env, jobject thiz,jobject surface) {
    pthread_mutex_lock(&mutex);

    //先释放之前的显示窗口
    if(window){
        ANativeWindow_release(window);
        window = 0;
    }
    //创建新的窗口用于视频显示
    window = ANativeWindow_fromSurface(env,surface);

    pthread_mutex_unlock(&mutex);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_myffmpeg_MyFfmpegPlayer_releaseNative(JNIEnv *env, jobject thiz) {
    pthread_mutex_lock(&mutex);

    if(window){
        //把老的释放
        ANativeWindow_release(window);
        window = 0;
    }

    pthread_mutex_unlock(&mutex);

    DELETE(ffmpeg);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_myffmpeg_MyFfmpegPlayer_stopNative(JNIEnv *env, jobject thiz) {
    if(ffmpeg){
        ffmpeg->stop();
    }
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_myffmpeg_MyFfmpegPlayer_getDurationNative(JNIEnv *env, jobject thiz) {
    if(ffmpeg){
        return ffmpeg->getDuration();
    }
    return 0;
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_myffmpeg_MyFfmpegPlayer_seekToNative(JNIEnv *env, jobject thiz,jint play_progress) {
    if(ffmpeg){
        ffmpeg->seekTo(play_progress);
    }
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_myffmpeg_MyFfmpegPlayer_pauseNative(JNIEnv *env, jobject thiz) {
    if(ffmpeg){
        ffmpeg->pause();
    }
}extern "C"
JNIEXPORT void JNICALL
Java_com_example_myffmpeg_MyFfmpegPlayer_resumeNative(JNIEnv *env, jobject thiz) {
    if(ffmpeg){
        ffmpeg->resume();
    }
}



