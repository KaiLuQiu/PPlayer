//
//  VideoRefreshThread.cpp
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//
#include "VideoRefreshThread.h"
#include "FrameQueueFunc.h"
#include <math.h>
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

void VideoRefreshThread::init(PlayerContext *playerContext)
{
    pPlayerContext = playerContext;
    
}

void VideoRefreshThread::start()
{
    thread video_refresh_thread([this]()-> void {              //此处使用lamda表达式
        run();
    });
    video_refresh_thread.detach();
}

int VideoRefreshThread::NeedAVSync()  // 在不考虑其他case的情况下都进行av sync
{
    return 1;
}

/* compute nominal last_duration */
double VideoRefreshThread::vp_duration(Frame *vp, Frame *nextvp) {  // 计算当前现实这笔的pts和下一笔的pts的差值，如果duration 满足如下几种情况，则使用包的duration， 否则使用差值duration
    if (vp->serial == nextvp->serial) {
        double duration = nextvp->pts - vp->pts;
        if (isnan(duration) || duration <= 0 || duration > pPlayerContext->max_frame_duration)
            return vp->duration;
        else
            return duration;
    } else {
        return 0.0;
    }
}

//int VideoRefreshThread::DecideKeepFrame(int need_av_sync, int64_t pts)
//{
//    int64_t late = 0;
//
//    if (!need_av_sync)
//        return 1;
//
//    late = CalcSyncLate(pts);
//    if (late < 0)
//    {
//        printf("video pts is late need drop this frame.\n");
//        //        qDebug()<<"video pts is late need drop this frame.";
//        return 0;
//    }
//
//    return 1;
//}


void VideoRefreshThread::run()
{
    int sync_status = FRAME_NEED_NEXT;
    int need_av_sync = 0;
    int bPaused = 0;
    double remaining_time = 0.0;    //每remaining_time运行一次循环（刷新一次屏幕）
    double time;
    double rdftspeed = 0.02;
    
    while (!needStop)
    {
        Frame *sp, *sp2;
        if (!pPlayerContext)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        if (remaining_time > 0.0) {
            av_usleep((int)(int64_t)(remaining_time * 1000000.0));
        }
        
        if (sync_status == FRAME_NEED_NEXT)
        {
            if (pPlayerContext->ic->streams[pPlayerContext->audioStreamIndex]) {
                time = av_gettime_relative() / 1000000.0;       // 获取当前的时间
                if (pPlayerContext->last_vis_time + rdftspeed < time) { // 如果
//                    video_display(is);
                    pPlayerContext->last_vis_time = time;
                }
                remaining_time = FFMIN(remaining_time, pPlayerContext->last_vis_time + rdftspeed - time);
            }
            if (pPlayerContext->ic->streams[pPlayerContext->videoStreamIndex]) // 说明当前存在视频流
            {
                if (FrameQueueFunc::frame_queue_nb_remaining(&pPlayerContext->videoDecodeRingBuffer) == 0) // 判断decoder queue中是否存在数据
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));       // 如果没有则，delay
                    continue;
                }
                else
                {
                    Frame *vp, *lastvp;
                    lastvp = FrameQueueFunc::frame_queue_peek_last(&pPlayerContext->videoDecodeRingBuffer); // 从frameQueue中获取当前播放器显示的帧
                    vp = FrameQueueFunc::frame_queue_peek(&pPlayerContext->videoDecodeRingBuffer); // 获取下一笔要现实的帧
                    if (vp->serial != pPlayerContext->videoRingBuffer.serial) {  // 当前的这笔数据流不连续，则跳过获取下一笔
                        FrameQueueFunc::frame_queue_next(&pPlayerContext->videoDecodeRingBuffer);
                        continue;
                    }
                    if (lastvp->serial != vp->serial)   // 如果上一笔和当前的这笔serial不对，表示不连续。这边应该从新获取frame_timer的时间
                        pPlayerContext->frame_timer = av_gettime_relative() / 1000000.0; //
                    
                    need_av_sync = NeedAVSync();
                    
                    
                }

            }
            
        }
    }
        
}

void VideoRefreshThread::stop()
{
    needStop = 1;
}
NS_MEDIA_END
