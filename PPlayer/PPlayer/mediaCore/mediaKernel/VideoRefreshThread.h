//
//  VedioRefreshThread.h
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#ifndef VideoRefreshThread_H
#define VideoRefreshThread_H
#include "MediaDefineInfo.h"
#include <thread>

NS_MEDIA_BEGIN

class VideoRefreshThread : public std::thread
{
public:
    static VideoRefreshThread *getIntanse()  // 饿汉模式
    {
        if(NULL == p_VideoOut) {
            SDL_LockMutex(mutex);
            if(NULL == p_VideoOut) {
                p_VideoOut = new (std::nothrow)VideoRefreshThread();
                if(p_VideoOut == NULL) {
                    printf("DemuxThread getInstance is NULL!");
                }
            }
            SDL_UnlockMutex(mutex);
        }
        return p_VideoOut;
    }
    void init();
    void start();
    void run();
    
    VideoRefreshThread();
    virtual ~VideoRefreshThread();
private:
    PlayerContext *pPlayerContext;
    static VideoRefreshThread* p_VideoOut;
    static SDL_mutex *mutex;
};


NS_MEDIA_END
#endif // VideoRefreshThread_H
