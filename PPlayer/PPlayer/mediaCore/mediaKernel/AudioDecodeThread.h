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
#include "mediaCore.h"

NS_MEDIA_BEGIN

class AudioDecodeThread : public std::thread
{
public:
    AudioDecodeThread();
    
    virtual ~AudioDecodeThread();
    
    /*
     * Audio解码线程的初始化过程
     */
    bool init(PlayerContext *playerContext, EventHandler *handler, mediaCore *p_Core);

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

private:
    PlayerContext   *pPlayerContext;
    EventHandler    *pHandler;
    mediaCore       *pMediaCore;
    int needStop;
};


NS_MEDIA_END
#endif // AudioDecodeThread_H
