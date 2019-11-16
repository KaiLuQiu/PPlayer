//
//  MediaDefineInfo.h
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//
#ifndef MediaDefineInfo_H
#define MediaDefineInfo_H
#include <SDL_mutex.h>
#include <list>

extern "C"{
#include<libavformat/avformat.h>
#include<libswscale/swscale.h>
#include<libswresample/swresample.h>
#include<libavutil/rational.h>
}


//enum AVMediaType {
//    AVMEDIA_TYPE_UNKNOWN = -1,  ///< Usually treated as AVMEDIA_TYPE_DATA
//    AVMEDIA_TYPE_VIDEO,
//    AVMEDIA_TYPE_AUDIO,
//    AVMEDIA_TYPE_DATA,          ///< Opaque data information usually continuous
//    AVMEDIA_TYPE_SUBTITLE,
//    AVMEDIA_TYPE_ATTACHMENT,    ///< Opaque data information usually sparse
//    AVMEDIA_TYPE_NB
//};


#define FRAME_QUEUE_SIZE 24

typedef struct P_AVPacket_T {
    AVPacket pkt;           //demuxer parse出来的包信息
    int serial;             //序号信息
} P_AVPacket;

//packtet队列信息
typedef struct PacketQueue_T {
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
    int freq;
    int channels;
    int64_t channel_layout;
    enum AVSampleFormat fmt;
    int frame_size;
    int bytes_per_sec;          //每秒的
} AudioInfo;

//解码帧信息
typedef struct Frame {
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
    AVCodecContext *video_avctx = NULL;
    AVCodecContext *audio_avctx = NULL;
}PlayerContext;


#endif // MediaDefineInfo_H
