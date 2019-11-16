//
//  MediaDefineInfo.h
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//
#ifndef MediaDefineInfo_H
#define MediaDefineInfo_H
#include <list>
#include <string.h>
#include <stdlib.h>

extern "C"{
#include<libavformat/avformat.h>
#include<libswscale/swscale.h>
#include<libswresample/swresample.h>
#include<libavutil/rational.h>
}
#include <SDL_mutex.h>
#include <SDL_thread.h>
#include <SDL_timer.h>
#include <SDL_mutex.h>
#include <SDL_audio.h>
#include <SDL_main.h>
#include <SDL_pixels.h>



#define FRAME_QUEUE_SIZE 24

#ifndef SAFE_DELETE
#define SAFE_DELETE(x) { if (x) delete (x); (x) = NULL; }    //定义安全释放函数
#endif
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(x) { if (x) delete [] (x); (x) = NULL; }    //定义安全释放函数
#endif
#ifndef SAFE_FREE
#define SAFE_FREE(p) if(p != NULL) {free(p); p = NULL;}
#endif

typedef struct P_AVPacket_T {
    P_AVPacket_T()
    {
        memset(&pkt, 0, sizeof(AVPacket));
        serial = 0;
    }
    ~P_AVPacket_T()
    {
        serial = 0;
    }
    AVPacket pkt;           //demuxer parse出来的包信息
    int serial;             //序号信息
} P_AVPacket;

//packtet队列信息
typedef struct PacketQueue_T {
    PacketQueue_T()
    {
        AvPacketList.clear();
        nb_packets = -1;
        size = -1;
        duration = -1;
        abort_request = -1;
        serial = -1;
        mutex = SDL_CreateMutex();
        cond = SDL_CreateCond();
    }
    ~PacketQueue_T()
    {
        std::list<P_AVPacket *>::iterator item = AvPacketList.begin();
        for(; item != AvPacketList.end(); )
        {
            std::list<P_AVPacket *>::iterator item_e = item++;
            SAFE_DELETE(*item_e);
            AvPacketList.erase(item_e);
        }
        AvPacketList.clear();
        
        nb_packets = -1;
        size = -1;
        duration = -1;
        abort_request = -1;
        serial = -1;
        SDL_DestroyMutex(mutex);
        SDL_DestroyCond(cond);
    }
    std::list<P_AVPacket *> AvPacketList;
    int nb_packets;         // 队列中packet的数量
    int size;               // 队列所占内存空间大小
    int64_t duration;       // 队列中所有packet总的播放时长
    int abort_request;
    int serial;             // 播放序列，所谓播放序列就是一段连续的播放动作，一个seek操作会启动一段新的播放序列
    SDL_mutex *mutex;
    SDL_cond *cond;
} PacketQueue;

typedef struct AudioInfo_T {
    AudioInfo_T()
    {
        freq = -1;
        channels = -1;
        channel_layout = -1;
        fmt = AV_SAMPLE_FMT_NONE;
        frame_size = -1;
        bytes_per_sec = -1;
    }
    ~AudioInfo_T()
    {
        freq = -1;
        channels = -1;
        channel_layout = -1;
        fmt = AV_SAMPLE_FMT_NONE;
        frame_size = -1;
        bytes_per_sec = -1;
    }
    int freq;
    int channels;
    int64_t channel_layout;
    enum AVSampleFormat fmt;
    int frame_size;
    int bytes_per_sec;          //每秒的
} AudioInfo;

//解码帧信息
typedef struct Frame {
    Frame()
    {
        frame = NULL;
        memset(&sub, 0, sizeof(AVSubtitle));
        serial = -1;
        pts = -1;
        duration = -1;
        pos = -1;
        width = -1;
        height = -1;
        format = -1;
        memset(&sar, 0, sizeof(AVRational));
        uploaded = -1;
        flip_v = -1;
    }
    ~Frame()
    {
        SAFE_DELETE(frame);
        memset(&sub, 0, sizeof(AVSubtitle));
        serial = -1;
        pts = -1;
        duration = -1;
        pos = -1;
        width = -1;
        height = -1;
        format = -1;
        memset(&sar, 0, sizeof(AVRational));
        uploaded = -1;
        flip_v = -1;
    }
    AVFrame *frame;
    AVSubtitle sub;
    int serial;
    double pts;           //解码时间戳
    double duration;      //帧时长信息
    int64_t pos;          //在码流的位置信息
    int width;            //视频的宽
    int height;           //视频的高
    int format;           //图像的格式
    AVRational sar;
    int uploaded;
    int flip_v;           //镜像
} Frame;

typedef struct FrameQueue {
    FrameQueue()
    {
        rindex = -1;
        windex = -1;
        size = -1;
        max_size = -1;
        keep_last = -1;
        rindex_shown = -1;
        mutex = SDL_CreateMutex();
        cond = SDL_CreateCond();
        pktq = NULL;
    }
    ~FrameQueue()
    {
        rindex = -1;
        windex = -1;
        size = -1;
        max_size = -1;
        keep_last = -1;
        rindex_shown = -1;
        SDL_DestroyMutex(mutex);
        SDL_DestroyCond(cond);
        SAFE_DELETE(pktq);
    }
    Frame queue[FRAME_QUEUE_SIZE];
    int rindex;
    int windex;
    int size;             // 总帧数
    int max_size;
    int keep_last;
    int rindex_shown;
    SDL_mutex *mutex;
    SDL_cond *cond;
    PacketQueue *pktq;
} FrameQueue;

typedef struct PlayerContext_T {
    PlayerContext_T()
    {
        avformat = NULL;
        seek_request = -1;
        seek_flags = -1;
        seek_pos = -1;
        seek_rel = -1;
        ic = NULL;
        audio_hw_buf_size = -1;
        keep_last = -1;                   //是否保存最后一帧
        width = -1;
        height = -1;
        video_avctx = NULL;
        audio_avctx = NULL;
    }
    ~PlayerContext_T()
    {
        SAFE_DELETE(avformat);
        seek_request = -1;
        seek_flags = -1;
        seek_pos = -1;
        seek_rel = -1;
        SAFE_DELETE(ic);
        audio_hw_buf_size = -1;
        keep_last = -1;                   //是否保存最后一帧
        width = -1;
        height = -1;
        SAFE_DELETE(video_avctx);
        SAFE_DELETE(audio_avctx);
    }
    AVInputFormat *avformat;        //
    int seek_request;               // 标识一次SEEK请求
    int seek_flags;                 // SEEK标志，诸如AVSEEK_FLAG_BYTE等
    int64_t seek_pos;               // SEEK的目标位置(当前位置+增量)
    int64_t seek_rel;               // 本次SEEK的位置增量
    AVFormatContext *ic;
    PacketQueue videoRingBuffer;    // 存储demuxer出来的未解码的序列帧
    PacketQueue audioRingBuffer;    // 存储demuxer出来的未解码的序列帧
    PacketQueue subtilteRingBuffer; // 存储demuxer出来的未解码的序列帧
    
    FrameQueue videoDecodeRingBuffer; // 存储decode出来的未解码的序列帧
    FrameQueue audioDecodeRingBuffer; // 存储decode出来的未解码的序列帧
    FrameQueue subDecodeRingBuffer;   // 存储decode出来的未解码的序列帧
    
    AudioInfo audioInfo;   // SDL支持的音频参数，重采样转换;
    int audio_hw_buf_size;          // SDL音频缓冲区大小(单位字节)
    bool keep_last;                   //是否保存最后一帧
    int width;                      
    int height;
    AVCodecContext *video_avctx;
    AVCodecContext *audio_avctx;
}PlayerContext;


#endif // MediaDefineInfo_H
