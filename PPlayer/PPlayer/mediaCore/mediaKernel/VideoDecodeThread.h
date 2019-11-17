//
//  VideoDecodeThread.h
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#ifndef VideoDecodeThread_H
#define VideoDecodeThread_H
#include "MediaDefineInfo.h"
#include <thread>

NS_MEDIA_BEGIN
class VideoDecodeThread : public std::thread
{
public:
    static VideoDecodeThread* getIntanse() {
        if(NULL == p_Decoder) {
            SDL_LockMutex(mutex);
            if(NULL == p_Decoder) {
                p_Decoder = new (std::nothrow)VideoDecodeThread();
                if(p_Decoder == NULL) {
                    printf("DemuxThread getInstance is NULL!");
                }
            }
            SDL_UnlockMutex(mutex);
        }
        return p_Decoder;
    }
    bool init(PlayerContext *playerContext);
    void run();
    void start();
    void stop();

    virtual ~VideoDecodeThread();
    VideoDecodeThread();
private:
    PlayerContext *pPlayerContext;
    static VideoDecodeThread* p_Decoder;
    static SDL_mutex *mutex;

    int bStop;
};


NS_MEDIA_END

#endif // VedioDecodeThread_H
