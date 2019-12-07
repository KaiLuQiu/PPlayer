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

#define SDL_AUDIO_MIN_BUFFER_SIZE 512
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30
#define MAX_AUDIO_FRAME_SIZE 192000


typedef enum {
    DISP_NONE,
    DISP_WAIT,
    DISP_DONE
} PCMBufferState_e;

typedef struct stPCMBuffer_T {
    stPCMBuffer_T() {
        bufferAddr = NULL;
    }
    ~stPCMBuffer_T() {
        SAFE_DELETE(bufferAddr);
    }
    uint8_t *bufferAddr;
    int64_t bufferSize;
    int64_t pts;
    PCMBufferState_e state;
} PCMBuffer;

typedef struct DispPCMQueue_T {
    DispPCMQueue_T() {
        rindex = 0;
        windex = 0;
        notUseNum = 0;
        size = 0;
        mutex = SDL_CreateMutex();
    }
    ~DispPCMQueue_T() {
        std::list<PCMBuffer *>::iterator item = Queue.begin();
        for(; item != Queue.end(); )
        {
            std::list<PCMBuffer *>::iterator item_e = item++;
            if(*item_e)
            {
                if(NULL != (*item_e)->bufferAddr)
                {
                    av_free((void *)(*item_e)->bufferAddr);
                    (*item_e)->bufferAddr = NULL;
                }
                (*item_e)->state = DISP_NONE;
                (*item_e)->bufferSize = 0;
                (*item_e)->pts = -1;
            }
            Queue.erase(item_e);
        }
        Queue.clear();
        rindex = 0;
        windex = 0;
        notUseNum = 0;
        size = 0;
        SDL_DestroyMutex(mutex);
    }
    std::list<PCMBuffer *> Queue;
    int rindex;
    int windex;
    int notUseNum;
    int size;
    SDL_mutex *mutex;
} DispPCMQueue;


class AudioRefreshThread : public std::thread {
public:
    /*
     * Audio输出现场的单例模式：饿汉模式
     */
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
    
    /*
     * Audio输出现场初始化过程
     */
    int init(PlayerContext *pPlayer);
    
    /*
     * 启动Audio输出
     */
    void start();
    
    /*
     * 重置Audio输出
     */
    void deinit();
    
    /*
     * audio输出的主要代码
     */
    void run();
    
    /*
     * 结束audio输出
     */
    void stop();
    
    /*
     * 重刷audio输出
     */
    void flush();
    virtual ~AudioRefreshThread();
    int bFirstFrame;
private:
    /*
     * SDL audio的callback
     */
    static void audio_callback(void *udata, unsigned char *stream, int len);
    
    /*
     * 获取一个有效的帧
     */
    Frame *GetOneValidFrame();
    
    /*
     * 获取一个可用的空PCM buffer
     */
    PCMBuffer *GetOneValidPCMBuffer();
private:
    static AudioRefreshThread *p_AudioOut;
    static SDL_mutex *mutex;
    PlayerContext *pPlayerContext;
    PCMBuffer PCMBuffers[FRAME_QUEUE_SIZE];
    DispPCMQueue ADispPCMQueue;
    int needStop;
    double audio_clock;
    int audio_clock_serial;
    int audio_hw_buf_size;              // audio设置的hardWareSize大小
    int buffer_size_index;              // 表示当前已经的读取的索引大小
    
};

NS_MEDIA_END

#endif // AudioRefreshThread_H
