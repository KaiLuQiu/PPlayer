//
//  VideoRefreshThread.cpp
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//
#include "VideoRefreshThread.h"

NS_MEDIA_BEGIN
SDL_mutex *VideoRefreshThread::mutex = SDL_CreateMutex();      //类的静态指针需要在此初始化
VideoRefreshThread* VideoRefreshThread::p_VideoOut = nullptr;

VideoRefreshThread::VideoRefreshThread()
{
    pPlayerContext = NULL;
//    bVideoFreeRun = 0;
//    pMasterClock = NULL;
    needStop = 0;

}

VideoRefreshThread::~VideoRefreshThread()
{
    
}

void VideoRefreshThread::init()
{
    
}

void VideoRefreshThread::start()
{
    thread video_refresh_thread([this]()-> void {              //此处使用lamda表达式
        run();
    });
    video_refresh_thread.detach();
}

void VideoRefreshThread::run()
{
    
}

void VideoRefreshThread::stop()
{
    needStop = 1;
}
NS_MEDIA_END
