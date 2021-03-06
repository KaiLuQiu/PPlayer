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
#include "EventHandler.h"
#include "mediaCore.h"

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
        sample_rate = 0;
        nb_samples = 0;
        pts = 0.0;
        bufferSize = 0;
        bufferAddr = NULL;
    }
    ~stPCMBuffer_T() {
        SAFE_DELETE(bufferAddr);
    }
    uint8_t *bufferAddr;
    int64_t bufferSize;
    double pts;
    int nb_samples;
    int sample_rate;
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
    AudioRefreshThread();
    
    virtual ~AudioRefreshThread();

    /*
     * Audio输出现场初始化过程
     */
    bool init(PlayerContext *pPlayer, EventHandler *handler, mediaCore *p_Core);
    
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
    
     /*
      * 将msg指令入队列
      */
    bool queueMessage(msgInfo msg);
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


    PCMBuffer PCMBuffers[PCM_QUEUE_SIZE];
    PCMBufferQueue pPCMBufferQueue;
    int needStop;
    message *pMessageQueue;                     // 当前的message信息
    msgInfo pCurMessage;                     // 当前的播放状态

    bool pPause;                               // 当前是否是pause状态
    bool pSeek;

    double audio_clock;
    int audio_clock_serial;
    int audio_hw_buf_size;                      // audio设置的hardWareSize大小
    int buffer_size_index;                      // 表示当前已经的读取的索引大小
    
    EventHandler    *pHandler;
    PlayerContext   *pPlayerContext;
    mediaCore       *pMediaCore;
};

NS_MEDIA_END

#endif // AudioRefreshThread_H
