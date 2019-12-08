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

NS_MEDIA_BEGIN
// 15M
#define MAX_SIZE (15 * 512 *1024)

class DemuxThread : public std::thread {
public:
    /*
     * demux线程的单例模式：饿汉模式
     */
    static DemuxThread* getIntanse() {
        if(NULL == pDemuxer) {
            SDL_LockMutex(mutex);
            if(NULL == pDemuxer) {
                pDemuxer = new (std::nothrow)DemuxThread();
                if(pDemuxer == NULL) {
                    printf("DemuxThread getInstance is NULL!\n");
                }
            }
            SDL_UnlockMutex(mutex);
        }
        return pDemuxer;
    }
    
    /*
     * demux线程的单例模式：饿汉模式
     */
    static double av_q2d(AVRational a) {
        return a.num / (double) a.den;
    }
    
    /*
     * demux线程的初始化过程
     */
    void init(PlayerContext *playerContext);
    
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
    bool queueMessage(MessageCmd msgInfo);
    virtual ~DemuxThread();
    DemuxThread();

private:
    static SDL_mutex *mutex;
    static DemuxThread* pDemuxer;
    bool pNeedStop;
    // 存储demuxer出来的未解码的序列帧
    PacketQueue *videoRingBuffer;
    // 存储demuxer出来的未解码的序列帧
    PacketQueue *audioRingBuffer;
    PlayerContext *pPlayerContext;
    PacketQueueFunc *videoPackeQueueFunc;
    PacketQueueFunc *audioPackeQueueFunc;
    // 当前的message信息
    message *pMessageQueue;

    AVPacket* video_flush_pkt;
    AVPacket* audio_flush_pkt;

    int duration = AV_NOPTS_VALUE;
    int start_time = AV_NOPTS_VALUE;
    // 流seek的方式有by byte也有by time
    int seek_by_bytes;
};


NS_MEDIA_END
#endif // DemuxThread_H
