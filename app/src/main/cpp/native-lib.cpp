#include <jni.h>
#include <string>
//=====================
#include "safe_queue.h"
#include "BaseChannel.h"
//=====================

extern "C" {
#include "libavcodec/avcodec.h"
}
//extern "C" JNIEXPORT jstring

extern "C" jstring
Java_com_example_myffmpeg_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++ ";
    return env->NewStringUTF(hello.c_str());
}
//extern "C"
//JNIEXPORT jstring

extern "C" jstring
Java_com_example_myffmpeg_MainActivity_getStringFromC(JNIEnv *env, jobject thiz) {
    return env->NewStringUTF("test test test 567   下面是ffmpeg的版本号：");
}
extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_myffmpeg_MainActivity_getVer(JNIEnv *env, jobject thiz) {
    return env->NewStringUTF(av_version_info());
}