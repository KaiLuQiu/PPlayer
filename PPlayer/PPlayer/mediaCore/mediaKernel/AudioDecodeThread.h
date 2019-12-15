//
//  AudioDecodeThread.h
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#ifndef AudioDecodeThread_H
#define AudioDecodeThread_H
#include "MediaDefineInfo.h"
#include <thread>
#include "EventHandler.h"

NS_MEDIA_BEGIN

class AudioDecodeThread : public std::thread
{
public:
    /*
     * Audio解码线程的单例模式：饿汉模式
     */
    static AudioDecodeThread* getIntanse() {
        if(NULL == p_Decoder) {
            SDL_LockMutex(mutex);
            if(NULL == p_Decoder) {
                p_Decoder = new (std::nothrow)AudioDecodeThread();
                if(p_Decoder == NULL) {
                    printf("AudioDecodeThread getInstance is NULL!\n");
                }
            }
            SDL_UnlockMutex(mutex);
        }
        return p_Decoder;
    }
    
    /*
     * Audio解码线程的初始化过程
     */
    bool init(PlayerContext *playerContext, EventHandler *handler);
    
    /*
     * Audio解码线程的主要运行代码
     */
    void run();
    
    /*
     * 开始audio解码
     */
    void start();
    
    /*
     * 停止解码
     */
    void stop();
    
    /*
     * 关闭解码
     */
    void close();
    
    /*
     * 对之前的解码数据流进行冲刷
     */
    void flush();
    
    /*
     * 获取一个audio解码帧
     */
    int get_audio_frame(AVFrame *frame);
    
    /*
     * 输入pkt，解码输出frame
     */
    int decoder_decode_frame(const AVPacket *AudioPkt, AVFrame *frame);
    
    /*
     * 将解码的frame推送到FrameQueue队列中
     */
    int queue_audio(AVFrame *src_frame, double pts, double duration, int64_t pos, int serial);
    virtual ~AudioDecodeThread();
    AudioDecodeThread();
private:
    static AudioDecodeThread *p_Decoder;
    static SDL_mutex *mutex;
    PlayerContext *pPlayerContext;
    EventHandler *pHandler;
    int needStop;
};


NS_MEDIA_END
#endif // AudioDecodeThread_H
