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
#include "AvSyncClock.h"

NS_MEDIA_BEGIN
SDL_mutex *VideoRefreshThread::mutex = SDL_CreateMutex();      //类的静态指针需要在此初始化
VideoRefreshThread* VideoRefreshThread::p_VideoOut = nullptr;

VideoRefreshThread::VideoRefreshThread()
{
    pPlayerContext = NULL;
    bVideoFreeRun = 0;
    needStop = 0;
    framedrop = -1;

}

VideoRefreshThread::~VideoRefreshThread()
{
    
}

void VideoRefreshThread::init(PlayerContext *playerContext)
{
    pPlayerContext = playerContext;
}

void VideoRefreshThread::setView(void *view)
{
    glView = view;
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


void VideoRefreshThread::update_video_pts(double pts, int64_t pos, int serial)
{
    AvSyncClock::set_clock(&pPlayerContext->VideoClock, pts, serial);
}

double VideoRefreshThread::compute_target_delay(double delay)
{
    double sync_threshold, diff = 0;

    // 计算video的时钟和audio时钟的差值
    diff = AvSyncClock::get_clock(&pPlayerContext->VideoClock) - AvSyncClock::get_master_clock(pPlayerContext);

    /* skip or repeat frame. We take into account the
       delay to compute the threshold. I still don't know
       if it is the best guess */
    sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
    if (!isnan(diff) && fabs(diff) < pPlayerContext->max_frame_duration) {
        // 如果当前落后audio 则应该丢帧
        if (diff <= -sync_threshold)
            delay = FFMAX(0, delay + diff);
        // 如果超前应该等待更久的时间
        else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD)
            delay = delay + diff;
        else if (diff >= sync_threshold)
            delay = 2 * delay;
    }

    printf("avsync: video refresh thread delay=%0.3f A-V=%f\n",
            delay, -diff);

    return delay;
}

// 计算当前现实这笔的pts和下一笔的pts的差值，如果duration 满足如下几种情况，则使用包的duration， 否则使用差值duration
double VideoRefreshThread::vp_duration(Frame *vp, Frame *nextvp) {
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
        // 目的是做音视频同步操作
        if (remaining_time > 0.0) {
            printf("avsync: video refresh thread need sleep time = %f\n", remaining_time);
            av_usleep((int)(int64_t)(remaining_time * 1000000.0));
        }
        if (sync_status == FRAME_NEED_NEXT)
        {
            // 说明当前存在视频流
            if (pPlayerContext->ic->streams[pPlayerContext->videoStreamIndex])
            {
                // 判断decoder queue中是否存在数据
                if (FrameQueueFunc::frame_queue_nb_remaining(&pPlayerContext->videoDecodeRingBuffer) == 0)
                {
                    // 如果没有则，delay
                    continue;
                }
                else
                {
                    double last_duration, duration, delay;
                    Frame *vp, *lastvp;
                    // 从frameQueue中获取当前播放器显示的帧
                    lastvp = FrameQueueFunc::frame_queue_peek_last(&pPlayerContext->videoDecodeRingBuffer);

                    // 获取下一笔要现实的帧
                    vp = FrameQueueFunc::frame_queue_peek(&pPlayerContext->videoDecodeRingBuffer);
                    
                    // 当前的这笔数据流不连续，则跳过获取下一笔
                    if (vp->serial != pPlayerContext->videoRingBuffer.serial) {
                        FrameQueueFunc::frame_queue_next(&pPlayerContext->videoDecodeRingBuffer);
                        continue;
                    }
                    // 如果上一笔和当前的这笔serial不对，表示不连续。这边应该从新获取frame_timer的时间
                    if (lastvp->serial != vp->serial)
                    {
                        pPlayerContext->frame_timer = av_gettime_relative() / 1000000.0; //
                    }
                    
                    need_av_sync = NeedAVSync();
                    if (need_av_sync)
                    {
                        // 计算上笔应该持续的时间
                        last_duration = vp_duration(lastvp, vp);

                        // 根据当前的视频和主时钟（audio时钟）计算差值diff,根据不同情况调整delay值
                        delay = compute_target_delay(last_duration);
                        printf("avsync: video refresh thread last duration = %f, delay = %f\n", last_duration, delay);

                        // 获取当前的系统时间值
                        time = av_gettime_relative() / 1000000.0;
                        
                        // 如果上一帧显示时长未满，重复显示上一帧
                        // 判断当前frame_timer + delay值是否大于当前的系统时间，如果大于计算剩余时间，继续显示当前帧
                        if (time < pPlayerContext->frame_timer + delay) {
                            remaining_time = FFMIN(pPlayerContext->frame_timer + delay - time, remaining_time);
                            video_image_display();
                            printf("avsync: video refresh thread show last frame time %f, frame_timer %f, delay %f\n", time, pPlayerContext->frame_timer, delay);
                            continue;
                        }
                        
                        // 更新frame_timer时间，frame_timer更新为上一帧结束时刻，也是当前帧开始时刻
                        pPlayerContext->frame_timer += delay;
                        // 如果delay大于0
                        if (delay > 0 && time - pPlayerContext->frame_timer > AV_SYNC_THRESHOLD_MAX)
                        {
                            printf("avsync: video refresh thread AV SYNC THRESHOLD MAX \n");
                            pPlayerContext->frame_timer = time;
                        }
                        
                        // 更新video clock
                        SDL_LockMutex(pPlayerContext->videoDecodeRingBuffer.mutex);
                        if (!isnan(vp->pts))
                        {
                            printf("avsync: video refresh thread video Clock = %f\n", vp->pts);
                            update_video_pts(vp->pts, vp->pos, vp->serial);
                        }
                        SDL_UnlockMutex(pPlayerContext->videoDecodeRingBuffer.mutex);
                        
                        // 丢帧模式
                        if (FrameQueueFunc::frame_queue_nb_remaining(&pPlayerContext->videoDecodeRingBuffer) > 1)
                        {
                            Frame *nextvp = FrameQueueFunc::frame_queue_peek_next(&pPlayerContext->videoDecodeRingBuffer);
                            duration = vp_duration(vp, nextvp);
                            printf("avsync: video refresh thread time %f, frame_timer %f, duration %f\n", time, pPlayerContext->frame_timer, duration);

                            // 如果当前时间要比这笔显示结束的时间（也就是下一笔开始时间）还大，则丢这一帧
                            if(time > pPlayerContext->frame_timer + duration)
                            {
                                printf("avsync: video refresh thread drop video frame_drops_late %d\n", pPlayerContext->frame_drops_late);
                                pPlayerContext->frame_drops_late++;
                                FrameQueueFunc::frame_queue_next(&pPlayerContext->videoDecodeRingBuffer);
                                continue;
                            }
                        }
                        
                        // 则正常显示
                        FrameQueueFunc::frame_queue_next(&pPlayerContext->videoDecodeRingBuffer);
                        video_image_display();
                    }
                    else
                    {
                        //暂时不做av sync操作，带音频模块的接入
                        FrameQueueFunc::frame_queue_next(&pPlayerContext->videoDecodeRingBuffer);
                        video_image_display();
                    }
                }
            }
        }
    }
        
}

void VideoRefreshThread::video_image_display()
{
    Frame *vp = FrameQueueFunc::frame_queue_peek_last(&pPlayerContext->videoDecodeRingBuffer);
    if (vp->frame) {
        enum AVPixelFormat sw_pix_fmt;
        sw_pix_fmt = pPlayerContext->videoDecoder->codecContext->sw_pix_fmt;
        if (sw_pix_fmt == AV_PIX_FMT_YUV420P || sw_pix_fmt == AV_PIX_FMT_YUVJ420P){
            VideoFrame *videoFrame = (VideoFrame *)malloc(sizeof(VideoFrame));
            videoFrame->width = vp->frame->width;
            videoFrame->height = vp->frame->height;
            videoFrame->format = AV_PIX_FMT_YUV420P;
            videoFrame->planar = 3;
            
            videoFrame->pixels[0] = (unsigned char *)malloc(vp->frame->width * vp->frame->height);
            videoFrame->pixels[1] = (unsigned char *)malloc(vp->frame->width * vp->frame->height);
            videoFrame->pixels[2] = (unsigned char *)malloc(vp->frame->width * vp->frame->height);
            copyYUVFrameData(vp->frame->data[0], videoFrame->pixels[0], vp->frame->linesize[0], vp->frame->width, vp->frame->height);
            copyYUVFrameData(vp->frame->data[1], videoFrame->pixels[1], vp->frame->linesize[1], vp->frame->width / 2, vp->frame->height / 2);
            copyYUVFrameData(vp->frame->data[2], videoFrame->pixels[2], vp->frame->linesize[2], vp->frame->width / 2, vp->frame->height / 2);
            Render(glView, videoFrame);
            
            free(videoFrame->pixels[0]);
            free(videoFrame->pixels[1]);
            free(videoFrame->pixels[2]);
            free(videoFrame);
        }
        
    }
}

void VideoRefreshThread::copyYUVFrameData(uint8_t *src, uint8_t *dst, int linesize, int width, int height){
    width = FFMIN(linesize, width);
    memset(dst, 0, width * height);
    for (int i = 0; i < height; ++i) {
        memcpy(dst, src, width);
        dst += width;
        src += linesize;
    }
}


void VideoRefreshThread::stop()
{
    needStop = 1;
}
NS_MEDIA_END
