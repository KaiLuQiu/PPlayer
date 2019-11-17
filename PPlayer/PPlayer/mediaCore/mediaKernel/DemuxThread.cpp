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

static AVPacket flush_pkt;                      //对于packetQueue中我们需要在一个向队列中先放置一个flush_pkt
                                                // 大概是为了每次seek操作后插入flush_pkt，更新serial，开启新的播放序列
DemuxThread::DemuxThread()
{
    seek_by_bytes = -1;
    pNeedStop = false;
}

DemuxThread::~DemuxThread()
{
}

void DemuxThread::init(PlayerContext *playerContext)
{
    pPlayerContext = playerContext;
    videoRingBuffer = &playerContext->videoRingBuffer;
    audioRingBuffer = &playerContext->audioRingBuffer;
    av_init_packet(&flush_pkt);
    flush_pkt.data = (uint8_t *)&flush_pkt;
}

void DemuxThread::flush()
{
    
}

void flushPacketQueue(PacketQueue *pPacketQueue)
{
    int i;
    int tmp = 0;
    P_AVPacket *pMyPkt = NULL;
    int listSize = pPacketQueue->AvPacketList.size();
    for (i = 0; i < listSize; i++)
    {
        if (!pPacketQueue->AvPacketList.empty())
        {
            pMyPkt = pPacketQueue->AvPacketList.front();
            pPacketQueue->AvPacketList.pop_front();
            if (pMyPkt && (pMyPkt->pkt.data != flush_pkt.data))
            {
                av_free_packet(&pMyPkt->pkt);
                free(pMyPkt);
            }
        }
    }
    pPacketQueue->AvPacketList.clear();
    
    return;
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
    
    while(!pNeedStop)
    {
        int a = 0;
    }
}

void DemuxThread::setSeekType(int type)
{
    seek_by_bytes = type;
}

NS_MEDIA_END
