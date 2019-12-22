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
#include "EventHandler.h"
#include "mediaCore.h"

NS_MEDIA_BEGIN
class VideoDecodeThread : public std::thread
{
public:
    VideoDecodeThread();

    virtual ~VideoDecodeThread();
    /*
     * Video解码线程的初始化过程
     */
    bool init(PlayerContext *playerContext, EventHandler *handler, mediaCore *p_Core);
    
    /*
     * Video解码线程主要运行代码
     */
    void run();
    
    /*
     * 启动Video解码
     */
    void start();
    
    /*
     * 结束Video解码
     */
    void stop();
    
    /*
     * 获取解码后的帧
     */
    int get_video_frame(AVFrame *frame);
    
    /*
     * 传入pkt，进行解码输出frame
     */
    int decoder_decode_frame(const AVPacket *VideoPkt, AVFrame *frame);
    
    /*
     * 将frame丢进FrameQueue中
     */
    int queue_picture(AVFrame *src_frame, double pts, double duration, int64_t pos, int serial);

private:
    PlayerContext   *pPlayerContext;
    EventHandler    *pHandler;
    mediaCore       *pMediaCore;

    int needStop;
};


NS_MEDIA_END

#endif // VedioDecodeThread_H
