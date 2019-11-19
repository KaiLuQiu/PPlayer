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
    frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(pPlayerContext->ic, pPlayerContext->ic->streams[pPlayerContext->videoStreamIndex], frame);
    
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
    AVPacket *VideoPkt;
    AVFrame *frame = av_frame_alloc();
    int serial = 0;
    AVRational frame_rate = av_guess_frame_rate(pPlayerContext->ic, pPlayerContext->ic->streams[pPlayerContext->videoStreamIndex], NULL);
    
    
    while(!bStop)
    {
        VideoPkt = new AVPacket();
        if(!pPlayerContext)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        if (pPlayerContext->videoRingBuffer.size < 0 || pPlayerContext->videoRingBuffer.AvPacketList.empty() ||
            pPlayerContext->videoRingBuffer.nb_packets == 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        if (pPlayerContext->videoRingBuffer.abort_request)          //
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        int ret = pPlayerContext->videoPacketQueueFunc->packet_queue_get(&pPlayerContext->videoRingBuffer, VideoPkt, 1, &pPlayerContext->videoDecoder->pkt_serial);
        if (ret < 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        if (VideoPkt->stream_index != pPlayerContext->videoStreamIndex)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        if(pPlayerContext->videoDecoder->pkt_serial != pPlayerContext->videoRingBuffer.serial);
        {
            continue;
        }
   
        
        ret = get_video_frame(VideoPkt, frame);
        if (ret < 0)
        {
            av_free_packet(VideoPkt);
            if (AVERROR_EOF == ret)
            {
                pPlayerContext->videoDecoder->finished = pPlayerContext->videoDecoder->pkt_serial;
                avcodec_flush_buffers(pPlayerContext->videoDecoder->codecContext);
                bStop = true;  //说明读取到了结束包信息
            }
            continue;
        }
        av_free_packet(VideoPkt);

        if (pPlayerContext->videoDecodeRingBuffer.Queue.size() < pPlayerContext->videoDecodeRingBuffer.size && pPlayerContext->videoDecodeRingBuffer.Queue.size() >= 0)
        {
            SDL_LockMutex(pPlayerContext->videoDecodeRingBuffer.mutex);
            
            int pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts;
            int duration = (frame_rate.num && frame_rate.den ? av_q2d((AVRational){frame_rate.den, frame_rate.num}) : 0);
            int64_t pos = frame->pkt_pos;

            queue_picture(frame, pts, duration, pos, pPlayerContext->videoDecoder->pkt_serial);

            SDL_UnlockMutex(pPlayerContext->videoDecodeRingBuffer.mutex);
        }

        if (VideoPkt != NULL)
        {
            free((void *)VideoPkt);
            VideoPkt = NULL;
        }
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

//    vp->sar = src_frame->sample_aspect_ratio;
//    vp->uploaded = 0;
//
//    vp->width = src_frame->width;
//    vp->height = src_frame->height;
//    vp->format = src_frame->format;
//
//    vp->pts = pts;
//    vp->duration = duration;
//    vp->pos = pos;
//    vp->serial = serial;
//    vp->frame = src_frame;
//    pPlayerContext->videoDecodeRingBuffer.Queue.push_back(vp);
    return 0;
}


void VideoDecodeThread::stop()
{
    
}


NS_MEDIA_END
