//
//  VedioDecodeThread.cpp
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//


#include "VideoDecodeThread.h"
#include "mediaCore.h"

NS_MEDIA_BEGIN
SDL_mutex *VideoDecodeThread::mutex = SDL_CreateMutex();      //类的静态指针需要在此初始化
VideoDecodeThread* VideoDecodeThread::p_Decoder = nullptr;

VideoDecodeThread::VideoDecodeThread()
{
    
}

VideoDecodeThread::~VideoDecodeThread()
{
    
}

int VideoDecodeThread::get_video_frame(const AVPacket *VideoPkt, AVFrame *frame)
{
    int ret = 0;
    ret = decoder_decode_frame(VideoPkt, frame);
    return ret;
}

int VideoDecodeThread::decoder_decode_frame(const AVPacket *VideoPkt, AVFrame *frame)
{
    int ret = mediaCore::getIntanse()->Decode(VideoPkt, frame);
    return ret;
}

bool VideoDecodeThread::init(PlayerContext *playerContext)
{
    pPlayerContext = playerContext;
    
    pPlayerContext->videoDecoder = new DecoderContext();
    
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
    bStop = 0;
    AVPacket *VideoPkt = NULL;
    AVFrame *frame = NULL;
    int serial = 0;
    
    while(bStop)
    {
        if(!pPlayerContext)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        if (pPlayerContext->videoRingBuffer.size < 0 || pPlayerContext->videoRingBuffer.AvPacketList.empty())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        if (pPlayerContext->videoRingBuffer.abort_request)          //
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        int ret = pPlayerContext->videoPacketQueueFunc->packet_queue_get(&pPlayerContext->videoRingBuffer, VideoPkt, 1, &serial);
        if (ret < 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        if(get_video_frame(VideoPkt, frame) < 0)
        {
            av_free_packet(VideoPkt);
            
        }
        av_free_packet(VideoPkt);
        
        
        

        
    }
}

void VideoDecodeThread::stop()
{
    
}


NS_MEDIA_END
