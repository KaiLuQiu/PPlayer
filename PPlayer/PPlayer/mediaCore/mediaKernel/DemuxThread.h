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

NS_MEDIA_BEGIN
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
    PacketQueue *videoRingBuffer;    // 存储demuxer出来的未解码的序列帧
    PacketQueue *audioRingBuffer;    // 存储demuxer出来的未解码的序列帧
    PlayerContext *pPlayerContext;
    int seek_by_bytes;  //流seek的方式有by byte也有by time
};


NS_MEDIA_END
#endif // DemuxThread_H
