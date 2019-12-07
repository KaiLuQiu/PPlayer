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

typedef enum {
    DISP_NONE,
    DISP_WAIT,
    DISP_DONE
} PCMBufferState;

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
    PCMBufferState state;
} PCMBuffer;

typedef struct PCMBufferQueue_T {
    PCMBufferQueue_T() {
        mutex = SDL_CreateMutex();
    }
    ~PCMBufferQueue_T() {
        // 这边可以直接clear,因为数据对象是指向PCMBuffers的元素的指针，而PCMBuffers是一个数组对象，里面的数据我会在对其进行统一释放
        Queue.clear();
        SDL_DestroyMutex(mutex);
    }
    std::list<PCMBuffer *> Queue;
    SDL_mutex *mutex;
} PCMBufferQueue;


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
    PCMBuffer PCMBuffers[PCM_QUEUE_SIZE];
    PCMBufferQueue pPCMBufferQueue;
    int needStop;
    double audio_clock;
    int audio_clock_serial;
    int audio_hw_buf_size;              // audio设置的hardWareSize大小
    int buffer_size_index;              // 表示当前已经的读取的索引大小
    
};

NS_MEDIA_END

#endif // AudioRefreshThread_H
