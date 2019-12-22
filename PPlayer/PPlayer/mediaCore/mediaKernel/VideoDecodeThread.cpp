//
//  VedioDecodeThread.cpp
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//


#include "VideoDecodeThread.h"
#include "FrameQueueFunc.h"
#include "AvSyncClock.h"

NS_MEDIA_BEGIN

VideoDecodeThread::VideoDecodeThread()
{
    pHandler = NULL;
}

VideoDecodeThread::~VideoDecodeThread()
{
    
}

int VideoDecodeThread::get_video_frame(AVFrame *frame)
{
    int ret = 0;
    double dpts = NAN;
    AVPacket VideoPkt;
    
    ret = pPlayerContext->videoPacketQueueFunc->packet_queue_get(&pPlayerContext->videoRingBuffer, &VideoPkt, 1, &pPlayerContext->videoDecoder->pkt_serial);
    if (ret < 0)
    {
        return ret;
    }
    
    // 如果当前读取到的序列号和队列序列号不一样说明当前连续，可能有seek过程导致
    if(pPlayerContext->videoDecoder->pkt_serial != pPlayerContext->videoRingBuffer.serial)
    {
        av_free_packet(&VideoPkt);
        return -1;
    }
    
    //如果当前这个拿到这个包pkt序列和queue里的packet序列不同，则代表过时的packet,
    if(VideoPkt.data == pPlayerContext->video_flush_pkt->data)         // 这个video flush pkt 目前我时直接指向demuxer线程创建的那个
    {
        // 表明当前这个pkt为flush_pkt，
        // 每当我们seek后，会在packet queue中先插入一个flush_pkt，更新当前serial，开启新的播放序列
        //那么就要复位解码内部的状态，刷新内部的缓冲区。因为有时候一个frame并不是由一个packet解出来的，那么可能当要播放新的序列
        // 信息时，还存有之前的packet包信息，所以要avcodec flush buffers
        avcodec_flush_buffers(pPlayerContext->videoDecoder->codecContext);
        pPlayerContext->videoDecoder->finished = 0;
        pPlayerContext->videoDecoder->next_pts = pPlayerContext->videoDecoder->start_pts;
        pPlayerContext->videoDecoder->next_pts_tb = pPlayerContext->videoDecoder->start_pts_tb;
        av_free_packet(&VideoPkt);
        return -1;
    }
    
    if (VideoPkt.stream_index != pPlayerContext->videoStreamIndex)
    {
        av_free_packet(&VideoPkt);
        return -1;
    }
    
    ret = decoder_decode_frame(&VideoPkt, frame);
    if (ret < 0)
    {
        av_free_packet(&VideoPkt);
        return ret;
    }
    
    if (frame->pts != AV_NOPTS_VALUE)
    {
        dpts = av_q2d(pPlayerContext->ic->streams[pPlayerContext->videoStreamIndex]->time_base) * frame->pts;
    }
    
    frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(pPlayerContext->ic, pPlayerContext->ic->streams[pPlayerContext->videoStreamIndex], frame);
    
    // 如果当前video 落后则丢帧处理
    if (frame->pts != AV_NOPTS_VALUE)
    {
        double diff = dpts - AvSyncClock::get_master_clock(pPlayerContext);
        
        if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD && diff < 0 &&
            pPlayerContext->videoDecoder->pkt_serial == pPlayerContext->VideoClock.serial &&
            pPlayerContext->videoRingBuffer.nb_packets)
        {
            printf("avsync:videodecode thread drop video, diff = %f, frame_drops_early %d\n", diff, pPlayerContext->frame_drops_early);
            pPlayerContext->frame_drops_early++;
            av_frame_unref(frame);
            av_free_packet(&VideoPkt);
            return -1;
        }
    }

    av_free_packet(&VideoPkt);
    return ret;
}

int VideoDecodeThread::decoder_decode_frame(const AVPacket *VideoPkt, AVFrame *frame)
{
    int ret = pMediaCore->Decode(VideoPkt, frame);
    return ret;
}

bool VideoDecodeThread::init(PlayerContext *playerContext, EventHandler *handler, mediaCore *p_Core)
{
    if (NULL == handler || NULL == playerContext || NULL == p_Core)
        return false;
    pPlayerContext = playerContext;
    pHandler = handler;
    pMediaCore = p_Core;
    if(pPlayerContext->videoPacketQueueFunc == NULL)
    {
        printf("pPlayerContext videoPacketQueueFunc is NULL \n");
        return false;
    }
    pPlayerContext->videoPacketQueueFunc->packet_queue_start(&pPlayerContext->videoRingBuffer);
    
    if (FrameQueueFunc::frame_queue_init(&pPlayerContext->videoDecodeRingBuffer, &pPlayerContext->videoRingBuffer, VIDEO_PICTURE_QUEUE_SIZE, 1) < 0)
    {
        printf("pPlayerContext frame queue init is NULL \n");
        return false;
    }
        
    // 初始化audio的同步时钟
    AvSyncClock::init_clock(&pPlayerContext->VideoClock, &pPlayerContext->videoRingBuffer.serial);
    
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
    needStop = 0;
    AVFrame *frame = av_frame_alloc();
    int serial = 0;
    // 设置好time_base
    AVRational tb = pPlayerContext->ic->streams[pPlayerContext->videoStreamIndex]->time_base;
    // 猜测视频帧率
    AVRational frame_rate = av_guess_frame_rate(pPlayerContext->ic, pPlayerContext->ic->streams[pPlayerContext->videoStreamIndex], NULL);

    while(!needStop)
    {
        if(!pPlayerContext)         // 判断上下文是否存在
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        if (pPlayerContext->videoRingBuffer.size < 0 || pPlayerContext->videoRingBuffer.AvPacketList.empty() ||
            pPlayerContext->videoRingBuffer.nb_packets == 0)        // 判断buffer中是否有包
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        if (pPlayerContext->videoRingBuffer.abort_request)          // 判断videoBuffer是否阻止获取信息
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        int ret = get_video_frame(frame);
        if(ret < 0)
        {
            if (AVERROR_EOF == ret)
            {
                pPlayerContext->videoDecoder->finished = pPlayerContext->videoDecoder->pkt_serial;
                avcodec_flush_buffers(pPlayerContext->videoDecoder->codecContext);  // 冲刷avcodec信息
                needStop = true;  //说明读取到了结束包信息
            }
            continue;
        }
        
        // 将pts时间转成秒
        // timestamp(ffmpeg内部时间戳) = AV_TIME_BASE * time(秒)
        // time(秒) = AV_TIME_BASE_Q * timestamp(ffmpeg内部时间戳)
        double pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
        double duration = (frame_rate.num && frame_rate.den ? av_q2d((AVRational){frame_rate.den, frame_rate.num}) : 0);
        int64_t pos = frame->pkt_pos;
        queue_picture(frame, pts, duration, pos, pPlayerContext->videoDecoder->pkt_serial);
        av_frame_unref(frame);
    }
}

int VideoDecodeThread::queue_picture(AVFrame *src_frame, double pts, double duration, int64_t pos, int serial)
{
    Frame *vp;
    
#if defined(DEBUG_SYNC)
    printf("frame_type=%c pts=%0.3f\n",
           av_get_picture_type_char(src_frame->pict_type), pts);
#endif
    // 从videoDecodeRingBuffer中获取一块Frame大小的可写内存，如果当前的size = max_size的话，说明写满了则返回为空
    if (!(vp = FrameQueueFunc::frame_queue_peek_writable(&pPlayerContext->videoDecodeRingBuffer)))
        return -1;
    
    vp->sar = src_frame->sample_aspect_ratio;
    vp->uploaded = 0;
    
    vp->width = src_frame->width;
    vp->height = src_frame->height;
    vp->format = src_frame->format;

    vp->pts = pts;
    vp->duration = duration;
    vp->pos = pos;
    vp->serial = serial;
    
    // 将src中包含的所有内容移至dst并重置src。
    av_frame_move_ref(vp->frame, src_frame);
    // 主要时将写索引往前移动
    FrameQueueFunc::frame_queue_push(&pPlayerContext->videoDecodeRingBuffer);
    // pPlayerContext->videoDecodeRingBuffer.Queue.push_back(vp);
    return 0;
}


void VideoDecodeThread::stop()
{
    needStop = 1;
}


NS_MEDIA_END
