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

class DemuxThread : public std::thread
{
public:
    static DemuxThread* getIntanse() {
        if(NULL == pDemuxer) {
            SDL_LockMutex(mutex);
            if(NULL == pDemuxer) {
                pDemuxer = new (std::nothrow)DemuxThread();
                if(pDemuxer == NULL) {
                    printf("DemuxThread getInstance is NULL!");
                }
            }
            SDL_UnlockMutex(mutex);
        }
        return pDemuxer;
    }
    static double av_q2d(AVRational a){
        return a.num / (double) a.den;
    }
    
    void init(PlayerContext *playerContext);
    void run();
    void seek();
    void start();
    void stop();
    void flush();

    void setSeekType(int type);

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

    AVPacket* video_flush_pkt;
    AVPacket* audio_flush_pkt;

    int duration = AV_NOPTS_VALUE;
    int start_time = AV_NOPTS_VALUE;

    // 流seek的方式有by byte也有by time
    int seek_by_bytes;
};


NS_MEDIA_END
#endif // DemuxThread_H
