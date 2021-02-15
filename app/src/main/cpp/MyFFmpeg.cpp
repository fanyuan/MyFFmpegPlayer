//
// Created by develop on 2021/2/15.
//

#include "MyFFmpeg.h"

MyFFmpeg::MyFFmpeg(JavaCallHelper *javaCallHelper, char *dataSource) {
    this->javaCallHelper = javaCallHelper;
    //    this->dataSource = dataSource;//?
    //这里的dataSource是从Java传过来的字符串，通过jni接口转成了c++字符串。
    //在jni方法中被释放掉了，导致 dataSource 变成悬空指针的问题（指向一块已经释放了的内存）
    //？？
    //内存拷贝，自己管理它的内存
    //strlen 获取字符串长度，strcpy：拷贝字符串

    //java: "hello"
    //c字符串以 \0 结尾 : "hello\0"
    this->dataSource = new char[strlen(dataSource) + 1];
    strcpy(this->dataSource,dataSource);
    pthread_mutex_init(&seekMutex,0);
}

MyFFmpeg::~MyFFmpeg() {
    DELETE(dataSource);
    DELETE(javaCallHelper);
    pthread_mutex_destroy(&seekMutex);
}

/**
 * 准备线程pid_prepare真正执行的函数
 * @param args
 * @return
 */
void *task_prepare(void *args){
    //打开输入
    MyFFmpeg *fFmpeg = static_cast<MyFFmpeg *>(args);

    fFmpeg->_prepare();
    return 0;//这里一定要返回0
}

void *task_start(void *args){
    MyFFmpeg *fFmpeg = static_cast<MyFFmpeg *>(args);
    fFmpeg->_start();
    return 0;//这里一定要返回0
}

/**
 * 设置为友元函数
 * @param args
 * @return
 */
void *task_stop(void *args){
    MyFFmpeg *fFmpeg = static_cast<MyFFmpeg *>(args);
    //要保证_prepare方法（子线程中）执行完再释放（在主线程）
    //pthread_join ：这里调用了后会阻塞主，可能引发ANR
    fFmpeg->isPlaying = 0;
    pthread_join(fFmpeg->pid_prepare,0);//解决了：要保证_prepare方法（子线程中）执行完再释放（在主线程）的问题
    pthread_join(fFmpeg->pid_start,0);
    LOGE("task_stop---000");
    if(fFmpeg->formatContext){
        avformat_close_input(&fFmpeg->formatContext);
        avformat_free_context(fFmpeg->formatContext);
        fFmpeg->formatContext = 0;
    }
    LOGE("task_stop---111");
    DELETE(fFmpeg->videoChannel);
    DELETE(fFmpeg->audioChannel);
    DELETE(fFmpeg);
    LOGE("task_stop---222");
    return 0;//这里一定要返回0
}

void MyFFmpeg::_prepare() {
    //0.5 AVFormatContext **ps
    formatContext = avformat_alloc_context();
    AVDictionary *dictionary = 0;
    av_dict_set(&dictionary,"timeout","10000000",0);//设置超时时间为10秒，这里的单位是微秒
    //1、打开媒体
    int ret = avformat_open_input(&formatContext,dataSource,0,&dictionary);
    av_dict_free(&dictionary);
    if(ret){
        LOGE("打开媒体失败：%s", av_err2str(ret));
        //失败 ，回调给java
        //        LOGE("打开媒体失败：%s", av_err2str(ret));
        //       javaCallHelper jni 回调java方法
        //       javaCallHelper->onError(ret);
        //          //可能java层需要根据errorCode来更新UI!
        //          //2019.8.12 作业自行尝试实现
        //TODO 反射通知java
        // 1 反射Java 属性（成员/静态）
        // 2 反射Java 方法 （成员/静态）
        // 3 子线程反射
        if(javaCallHelper){
            javaCallHelper->onError(THREAD_CHILD,FFMPEG_CAN_NOT_OPEN_URL);
        }
        return;
    }
    //2、查找媒体中的流信息
    ret = avformat_find_stream_info(formatContext,0);
    if(ret < 0){
        LOGE("查找媒体中的流信息失败：%s", av_err2str(ret));
        //TODO 反射通知java
        if (javaCallHelper) {
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS);
        }
        return;
    }
    duration = formatContext->duration/AV_TIME_BASE;
    //这里的 i 就是后面 用到 的 packet->stream_index
    for(int i = 0;i < formatContext->nb_streams; ++i){
        //3、获取媒体流（音频或者视频）
        AVStream *stream = formatContext->streams[i];
        //4、获取编码解码这段流的参数
        AVCodecParameters *codecParameters = stream->codecpar;
        //5、通过参数中的id（编解码的方式），来查找当前流的解码器
        AVCodec *codec = avcodec_find_decoder(codecParameters->codec_id);
        if(!codec){
            LOGE("查找当前流的解码器失败");
            //TODO 反射通知java
            if (javaCallHelper) {
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_FIND_DECODER_FAIL);
            }
            return;
        }
        //6、创建解码器上下文
        AVCodecContext *codecContext = avcodec_alloc_context3(codec);
        if(!codecContext){
            LOGE("创建解码器上下文失败");
            if (javaCallHelper) {
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
            }
        }
        //7、设置解码器上下文的参数
        ret = avcodec_parameters_to_context(codecContext,codecParameters);
        if(ret < 0){
            //TODO 反射通知java
            LOGE("设置解码器上下文的参数失败：%s", av_err2str(ret));
            if (javaCallHelper) {
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL);
            }
            return;
        }
        //8、打开解码器
        ret = avcodec_open2(codecContext,codec,0);
        if(ret){
            //TODO 反射通知java
            LOGE("打开解码器失败：%s", av_err2str(ret));
            if (javaCallHelper) {
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL);
            }
            return;
        }
        AVRational time_base = stream -> time_base;
        //判断流类型（音频还是视频）
        if(codecParameters->codec_type == AVMEDIA_TYPE_AUDIO){
            //音频
            audioChannel = new AudioChannel(i,codecContext,time_base,javaCallHelper);
        } else if(codecParameters->codec_type == AVMEDIA_TYPE_VIDEO){
            //视频
            AVRational frame_rate = stream->avg_frame_rate;
            // int fps = fram_rate.num / fram_rate.den;
            int fps = av_q2d(frame_rate);

            videoChannel = new VideoChannel(i,codecContext,fps,time_base,javaCallHelper);
            videoChannel->setRenderCallback(renderCallback);
        }
    }//end for

    if(!audioChannel && !videoChannel){
        //既没有音频也没有视频
        //TODO 反射通知java
        LOGE("没有音视频");
        if (javaCallHelper) {
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_NOMEDIA);
        }
        return;
    }
    //准备好了，反射通知java
    if(javaCallHelper){
        javaCallHelper->onPrepared(THREAD_CHILD);
    }
}
/**
 * 播放准备
 * 可能是主线程
 * doc/samples/
 */
void MyFFmpeg::prepare() {

        //可以直接来进行解码api调用吗？
        //xxxxxx。。。。不能！
        //文件：io流问题
        //直播：网络
//    pthread_create： 创建子线程

//pthread_create(pthread_t* __pthread_ptr, pthread_attr_t const* __attr, void* (*__start_routine)(void*), void*);
   pthread_create(&pid_prepare,0,task_prepare,this);
}
/**
 * 开始播放
 */
void MyFFmpeg::start() {
    isPlaying = 1;
    isPause = 0;
    if(videoChannel){
        videoChannel->setAudioChannel(audioChannel);
        videoChannel->start();
    }
    if(audioChannel){
        audioChannel->start();
    }
    pthread_create(&pid_start,0,task_start,this);
}
/**
 * 真正执行解码播放
 */
void MyFFmpeg::_start() {
    while (isPlaying){

        if(isPause){
            continue;
        }

        /**
         * 内存泄漏点
         * 控制 视频 packets 队列
         */
         if(videoChannel && videoChannel->packets.size() > 100){
             av_usleep(10 * 1000);
             continue;
         }
        //内存泄漏点（控制 音频 packets 队列）
         if(audioChannel && audioChannel->packets.size() > 100){
             av_usleep(10 * 1000);
             continue;
         }
         AVPacket *packet = av_packet_alloc();
         pthread_mutex_lock(&seekMutex);
         int ret = av_read_frame(formatContext,packet);
         pthread_mutex_unlock(&seekMutex);
         if(!ret){
             //ret 为 0 表示成功
             //要判断流类型，是视频还是音频
             if(videoChannel && packet->stream_index == videoChannel->id){
                 //往视频编码数据包队列中添加数据
                 videoChannel->packets.push(packet);
             } else if(audioChannel && packet->stream_index == audioChannel->id){
                 //往音频编码数据包队列中添加数据
                 audioChannel->packets.push(packet);
             }
         } else if(ret == AVERROR_EOF){
             //表示读完了
             //要考虑读完了，是否播完了的情况
             //如何判断有没有播放完？判断队列是否为空
             if(videoChannel->packets.empty() && videoChannel->frames.empty()
                 && audioChannel->packets.empty() && audioChannel->frames.empty()){
                 //播完了
                 av_packet_free(&packet);
                 break;
             }
         } else{
             //TODO 反射通知java
             LOGE("读取音视频数据包失败");
             if (javaCallHelper) {
                 javaCallHelper->onError(THREAD_CHILD, FFMPEG_READ_PACKETS_FAIL);
             }
             av_packet_free(&packet);
             break;
         }
    }
    isPlaying = 0;
    //停止解码播放（音频和视频）
    videoChannel->stop();
    audioChannel->stop();
}

void MyFFmpeg::setRenderCallback(RenderCallback renderCallback) {
    this->renderCallback = renderCallback;
}
void MyFFmpeg::pause() {
    isPause = 1;
    if(videoChannel){
        videoChannel->packets.setWork(0);
        videoChannel->frames.setWork(0);
        videoChannel->packets.clear();
        videoChannel->frames.clear();
    }
    if(audioChannel){
        audioChannel->packets.setWork(0);
        audioChannel->frames.setWork(0);
        audioChannel->packets.clear();
        audioChannel->frames.clear();
    }
}
void MyFFmpeg::resume() {
    isPause = 0;
    if(videoChannel){
        videoChannel->packets.setWork(1);
        videoChannel->frames.setWork(1);
    }
    if(audioChannel){
        audioChannel->packets.setWork(1);
        audioChannel->frames.setWork(1);
    }
}
/**
 * 停止播放
 */
void MyFFmpeg::stop() {
    javaCallHelper = 0;//prepare阻塞中停止了，还是会回调给java "准备好了"

    //既然在主线程会引发ANR，那么我们到子线程中去释放
    pthread_create(&pid_stop,0,task_stop,this);//创建stop子线程

//    if (formatContext) {
//        avformat_close_input(&formatContext);
//        avformat_free_context(formatContext);
//        formatContext = 0;
//    }
//    if (videoChannel) {
//        videoChannel->stop();
//    }
//    if (audioChannel) {
//        audioChannel->stop();
//    }
}

int MyFFmpeg::getDuration() const {
    return duration;
}

void MyFFmpeg::seekTo(int playProgress) {
    if(playProgress < 0 || playProgress > duration){
        LOGE("指定的progress不合法：%d",playProgress);
        return;
    }
    if(!audioChannel && !videoChannel){
        LOGE("!audioChannel && !videoChannel");
        return;
    }
    if(!formatContext){
        LOGE("seekTo上下文为null");
        return;
    }
    //1,上下文
    //2，流索引，-1：表示选择的是默认流
    //3，要seek到的时间戳
    //4，seek的方式
    //AVSEEK_FLAG_BACKWARD： 表示seek到请求的时间戳之前的最靠近的一个关键帧
    //AVSEEK_FLAG_BYTE：基于字节位置seek
    //AVSEEK_FLAG_ANY：任意帧（可能不是关键帧，会花屏）
    //AVSEEK_FLAG_FRAME：基于帧数seek
    pthread_mutex_lock(&seekMutex);
    int ret = av_seek_frame(formatContext,-1,playProgress*AV_TIME_BASE,AVSEEK_FLAG_BACKWARD);

    if(ret < 0){
        if(javaCallHelper){
            javaCallHelper->onError(THREAD_CHILD,ret);
        }
    }
    if(audioChannel){
        audioChannel->packets.setWork(0);
        audioChannel->frames.setWork(0);
        audioChannel->packets.clear();
        audioChannel->frames.clear();
        //清除数据后，让队列重新工作
        audioChannel->packets.setWork(1);
        audioChannel->frames.setWork(1);
    }
    if(videoChannel){
        videoChannel->packets.setWork(0);
        videoChannel->frames.setWork(0);
        videoChannel->packets.clear();
        videoChannel->frames.clear();
        //清除数据后，让队列重新工作
        videoChannel->packets.setWork(1);
        videoChannel->frames.setWork(1);
    }

    pthread_mutex_unlock(&seekMutex);
}


