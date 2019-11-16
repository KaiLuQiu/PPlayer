//
//  DemuxThread.cpp
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#include "Demuxthread.h"
SDL_mutex *DemuxThread::mutex = SDL_CreateMutex();      //类的静态指针需要在此初始化


DemuxThread::DemuxThread()
{
    seek_by_bytes = -1;
    pNeedStop = false;
}

DemuxThread::~DemuxThread()
{
}

void DemuxThread::start()
{
    thread t([this]()-> void {
        run();
    });
    t.detach();
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
        
    }
}

void DemuxThread::setSeekType(int type)
{
    seek_by_bytes = type;
}
