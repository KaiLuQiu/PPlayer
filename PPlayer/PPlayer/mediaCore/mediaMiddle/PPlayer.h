//
//  PPlayer.h
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#ifndef PPLAYER_H
#define PPLAYER_H

#include <list>
#include <string>
#include <SDL_mutex.h>
#include "MediaDefineInfo.h"

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
}PlayerContext;

class PPlayer
{
public:
    static PPlayer *getInstance() {         //懒汉模式
        if(NULL == p_Player) {
            SDL_LockMutex(mutex);
            if(NULL == p_Player) {
                p_Player = new PPlayer();
            }
            SDL_UnlockMutex(mutex);
        }
        return p_Player;
    }
    
    bool setDataSource(std::string url);
    bool prepareAsync();
    bool prepare();
    bool start();
    bool pause();
    bool seek(int64_t pos);
    bool resume();
    bool stop();
    void flush();
    bool setLoop(bool loop);
    int getCurPos();
    bool setSpeed();
    float getSpeed();
    
    PlayerContext* getPlayerContext()      //写在class内相当于内联函数
    {
        return pPlayerContext;
    }
    
    virtual ~PPlayer();     //析构函数一定不能私有话，否则可能导致内存泄漏
private:
    PPlayer();      //因为是单例模式，构造函数可以私有化
    
    PlayerContext *pPlayerContext;
    
    static PPlayer *p_Player;
    static SDL_mutex *mutex;
};


#endif // PPLAYER_H
