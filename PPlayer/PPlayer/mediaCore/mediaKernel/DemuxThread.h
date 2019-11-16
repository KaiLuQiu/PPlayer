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
    void run();
    void seek();
    void start();
    void stop();
    void setSeekType();
    
    virtual ~DemuxThread();
    DemuxThread();
private:
    static SDL_mutex *mutex;
    static DemuxThread* pDemuxer;
    bool pNeedStop;
    int seek_by_bytes;  //流seek的方式有by byte也有by time
};


#endif // DemuxThread_H
