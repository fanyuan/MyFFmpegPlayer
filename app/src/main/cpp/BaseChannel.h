//
// Created by develop on 2021/2/14.
//

#ifndef MYFFMPEGVIDEO_BASECHANNEL_H
#define MYFFMPEGVIDEO_BASECHANNEL_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libavutil/time.h>
};

#include "safe_queue.h"
#include "JavaCallHelper.h"

/**
 * VideoChannel和AudioChannel的父类
 */
class BaseChannel {
public:
    BaseChannel(int id,AVCodecContext *codecContext,AVRational time_base,
            JavaCallHelper *javaCallHelper):id(id),codecContext(codecContext),
            time_base(time_base),javaCallHelper(javaCallHelper){
        //packets.setReleaseCallback(relea)

        //TODO bug
        packets.setReleaseCallback(releaseAVPacket);
        frames.setReleaseCallback(releaseAVFrame);
    }

    virtual ~BaseChannel() {
        //TODO bug
        packets.clear();
        frames.clear();
        if (codecContext) {
            avcodec_close(codecContext);
            avcodec_free_context(&codecContext);
            codecContext = 0;
        }
    }

    /**
     * 释放 AVPacket
     * @param packet
     */
    static void releaseAVPacket(AVPacket **packet){
        if(packet){
            av_packet_free(packet);
            *packet = 0;
        }
    }
    /**
     * 释放 AVFrame
     * @param frame
     */
    static void releaseAVFrame(AVFrame **frame){
        if(frame){
            av_frame_free(frame);
            *frame = 0;
        }
    }
    /**
     * 纯虚函数（抽象方法）
     */
    virtual void start() = 0;
    /**
     * 纯虚函数（抽象方法）
     */
    virtual void stop() = 0;

    SafeQueue<AVPacket *> packets;
    SafeQueue<AVFrame *> frames;
    int id;
    bool isPlaying = 0;
    //解码器上下文
    AVCodecContext *codecContext;
    AVRational time_base;
    double audio_time;
    JavaCallHelper *javaCallHelper = 0;
};


#endif //MYFFMPEGVIDEO_BASECHANNEL_H