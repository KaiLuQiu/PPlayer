//
//  DemuxThread.h
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#ifndef DemuxThread_H
#define DemuxThread_H
#include <thread>
#include "MediaDefineInfo.h"
#include "PacketQueueFunc.h"
#include "EventHandler.h"
#include "mediaCore.h"

NS_MEDIA_BEGIN
// 15M
#define MAX_SIZE (15 * 512 *1024)

class DemuxThread : public std::thread {
public:
    DemuxThread();
    
    virtual ~DemuxThread();

    /*
     * demux线程的单例模式：饿汉模式
     */
    static double av_q2d(AVRational a) {
        return a.num / (double) a.den;
    }
    
    /*
     * demux线程的初始化过程
     */
    bool init(PlayerContext *playerContext, EventHandler *handler, mediaCore *p_Core);

    /*
     * demux线程主要运行的代码
     */
    void run();
    
    /*
     * 开启demuxer过程
     */
    void start();
    
    /*
     * 结束demuxer过程
     */
    void stop();
    
    /*
     * 冲刷demuxer，将parse出来的包进行重刷掉
     */
    void flush();

    /*
     * 设置seek的方式
     */
    void setSeekType(int type);

    /*
     * 将msg指令入队列
     */
    bool queueMessage(msgInfo msg);

private:
    bool pNeedStop;
    // 存储demuxer出来的未解码的序列帧
    PacketQueue *videoRingBuffer;
    // 存储demuxer出来的未解码的序列帧
    PacketQueue *audioRingBuffer;
    PacketQueueFunc *videoPackeQueueFunc;
    PacketQueueFunc *audioPackeQueueFunc;
    // 当前的message信息
    message *pMessageQueue;
    // 当前的播放状态
    msgInfo pCurMessage;
    bool    pSeek;
    float   pSeekPos;

    AVPacket* video_flush_pkt;
    AVPacket* audio_flush_pkt;

    int duration = AV_NOPTS_VALUE;
    int start_time = AV_NOPTS_VALUE;
    // 流seek的方式有by byte也有by time
    int seek_by_bytes;
    
    PlayerContext       *pPlayerContext;
    EventHandler        *pHandler;
    mediaCore           *pMediaCore;
    SDL_mutex           *pMutex;
    
};


NS_MEDIA_END
#endif // DemuxThread_H
