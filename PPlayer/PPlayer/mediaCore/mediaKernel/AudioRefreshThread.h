//
//  AudioRefreshThread.h
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#ifndef AudioRefreshThread_H
#define AudioRefreshThread_H
#include "MediaDefineInfo.h"
#include <thread>
#include "stdint.h"

NS_MEDIA_BEGIN

typedef enum
{
    DISP_NONE,
    DISP_WAIT,
    DISP_DONE
} PCMBufferState_e;

typedef struct stPCMBuffer_T
{
    stPCMBuffer_T()
    {
        bufferAddr = NULL;
    }
    ~stPCMBuffer_T()
    {
        SAFE_DELETE(bufferAddr);
    }
    char *bufferAddr;
    int64_t bufferSize;
    int64_t pts;
    PCMBufferState_e state;
} PCMBuffer_t;

class AudioRefreshThread : public std::thread
{
public:
    static AudioRefreshThread* getIntanse() {
        if(NULL == p_AudioOut) {
            SDL_LockMutex(mutex);
            if(NULL == p_AudioOut) {
                p_AudioOut = new (std::nothrow)AudioRefreshThread();
                if(p_AudioOut == NULL) {
                    printf("AudioDecodeThread getInstance is NULL!");
                }
            }
            SDL_UnlockMutex(mutex);
        }
        return p_AudioOut;
    }
    AudioRefreshThread();
    void init(PlayerContext *pPlayer);
    void start();
    void deinit();
    void run();
    void stop();
    void flush();
    virtual ~AudioRefreshThread();
    int bFirstFrame;
private:
    static AudioRefreshThread *p_AudioOut;
    static SDL_mutex *mutex;
    PlayerContext *pPlayerContext;
    PCMBuffer_t PCMBuffers[FRAME_QUEUE_SIZE];
    int needStop;
    PCMBuffer_t *GetOneValidPCMBuffer();
};


NS_MEDIA_END

#endif // AudioRefreshThread_H
