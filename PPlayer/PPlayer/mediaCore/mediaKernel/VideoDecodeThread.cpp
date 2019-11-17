//
//  VedioDecodeThread.cpp
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//


#include "VideoDecodeThread.h"

NS_MEDIA_BEGIN
SDL_mutex *VideoDecodeThread::mutex = SDL_CreateMutex();      //类的静态指针需要在此初始化
VideoDecodeThread* VideoDecodeThread::p_Decoder = nullptr;

VideoDecodeThread::VideoDecodeThread()
{
    
}

VideoDecodeThread::~VideoDecodeThread()
{
    
}

bool VideoDecodeThread::init(PlayerContext *playerContext)
{
    pPlayerContext = playerContext;
    if(pPlayerContext->videoPacketQueueFunc == NULL)
    {
        printf("pPlayerContext videoPacketQueueFunc is NULL \n");
        return false;
    }
    pPlayerContext->videoPacketQueueFunc->packet_queue_start(&pPlayerContext->videoRingBuffer);
    
    if(pPlayerContext->audioPacketQueueFunc == NULL)
    {
        printf("pPlayerContext audioPacketQueueFunc is NULL \n");

        return false;
    }
    pPlayerContext->audioPacketQueueFunc->packet_queue_start(&pPlayerContext->audioRingBuffer);
    return true;
}

void VideoDecodeThread::start()
{
    thread video_decode_thread([this]()-> void {              //此处使用lamda表达式
        run();
    });
    video_decode_thread.detach();
}

void VideoDecodeThread::run()
{
    while(1)
    {
        int a = 0;
    }
}

void VideoDecodeThread::stop()
{
    
}


NS_MEDIA_END
