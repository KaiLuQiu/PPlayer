//
//  DemuxThread.cpp
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#include "Demuxthread.h"

NS_MEDIA_BEGIN
SDL_mutex *DemuxThread::mutex = SDL_CreateMutex();      //类的静态指针需要在此初始化
DemuxThread* DemuxThread::pDemuxer = nullptr;

// 对于packetQueue中我们需要在一个向队列中先放置一个flush_pkt，主要用来作为非连续的两端数据的“分界”标记
// 大概是为了每次seek操作后插入flush_pkt，更新serial，开启新的播放序列

DemuxThread::DemuxThread()
{
    seek_by_bytes = -1;
    pNeedStop = false;
    videoPackeQueueFunc = NULL;
    audioPackeQueueFunc = NULL;
}

DemuxThread::~DemuxThread()
{
}

void DemuxThread::init(PlayerContext *playerContext)
{
    pPlayerContext = playerContext;
    videoRingBuffer = &playerContext->videoRingBuffer;
    audioRingBuffer = &playerContext->audioRingBuffer;

    av_init_packet(&video_flush_pkt);
    video_flush_pkt.data = (uint8_t *)&video_flush_pkt;
    
    av_init_packet(&audio_flush_pkt);
    audio_flush_pkt.data = (uint8_t *)&audio_flush_pkt;
    
    if(pPlayerContext->videoStreamIndex >= 0)
    {
        pPlayerContext->videoPacketQueueFunc = new (std::nothrow)PacketQueueFunc(&video_flush_pkt);
        if(!pPlayerContext->videoPacketQueueFunc)
        {
            printf("pPlayerContext->videoPackeQueueFunc error!\n");
        }
        videoPackeQueueFunc = pPlayerContext->videoPacketQueueFunc;
        
    }
    if(pPlayerContext->audioStreamIndex >= 0)
    {
        pPlayerContext->audioPacketQueueFunc = new (std::nothrow)PacketQueueFunc(&audio_flush_pkt);
        if(!pPlayerContext->audioPacketQueueFunc)
        {
            printf("pPlayerContext->audioPacketQueueFunc error!\n");
        }
        audioPackeQueueFunc = pPlayerContext->audioPacketQueueFunc;
    }
    
    videoPackeQueueFunc->packet_queue_init(videoRingBuffer);
    audioPackeQueueFunc->packet_queue_init(audioRingBuffer);
}

void DemuxThread::flush()
{
    
}

void DemuxThread::start()
{
    thread read_thread([this]()-> void {              //此处使用lamda表达式
        run();
    });
    read_thread.detach();
}

void DemuxThread::stop()
{
    pNeedStop = true;
}

void DemuxThread::run()
{
    int ret = -1;
    int64_t stream_start_time;          //流的开始时间
    int pkt_in_play_range = 0;          //当前的duation时间
    AVDictionaryEntry *t;
    AVPacket pkt;
    memset(&pkt, 0, sizeof(AVPacket));
    
    while(!pNeedStop)
    {
        if(!pPlayerContext)             //表示当前的播放器上下文还没有准备好，可以先delay10ms
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            printf("pPlayerContext is NUll");
            continue;
        }
        if(videoPackeQueueFunc == NULL || audioPackeQueueFunc == NULL)
        {
            printf("PackeQueueFunc is NUll");
            break;
        }
        if(videoRingBuffer->size + audioRingBuffer->size > MAX_SIZE)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            printf("ringbuffer is full");
            continue;
        }
        
        int ret = av_read_frame(pPlayerContext->ic, &pkt);

        if(ret < 0)
        {
            if ((ret == AVERROR_EOF || avio_feof(pPlayerContext->ic->pb)) && !pPlayerContext->eof) {
                if (pPlayerContext->videoStreamIndex >= 0)
                    videoPackeQueueFunc->packet_queue_put_nullpacket(videoRingBuffer, pPlayerContext->videoStreamIndex);
                if (pPlayerContext->audioStreamIndex >= 0)
                    audioPackeQueueFunc->packet_queue_put_nullpacket(audioRingBuffer, pPlayerContext->audioStreamIndex);
                pPlayerContext->eof = 1;
            }
            if (pPlayerContext->ic->pb && pPlayerContext->ic->pb->error)
                break;
            //让线程等待10ms
            SDL_LockMutex(mutex);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            SDL_UnlockMutex(mutex);
            continue;
        }
        else
        {
            pPlayerContext->eof = 0;
        }

        if (pkt.stream_index == pPlayerContext->audioStreamIndex)
        {
            audioPackeQueueFunc->packet_queue_put(audioRingBuffer, &pkt);
        }
        else if (pkt.stream_index == pPlayerContext->videoStreamIndex)
        {
            videoPackeQueueFunc->packet_queue_put(videoRingBuffer, &pkt);
        }
        else
        {
            av_packet_unref(&pkt);
        }
                
        
    }
}

void DemuxThread::setSeekType(int type)
{
    seek_by_bytes = type;
}

NS_MEDIA_END
