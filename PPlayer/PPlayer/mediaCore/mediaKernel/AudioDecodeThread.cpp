//
//  AudioDecodeThread.cpp
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#include "AudioDecodeThread.h"
#include "mediaCore.h"
#include "FrameQueueFunc.h"

NS_MEDIA_BEGIN
SDL_mutex *AudioDecodeThread::mutex = SDL_CreateMutex();      //类的静态指针需要在此初始化
AudioDecodeThread* AudioDecodeThread::p_Decoder = nullptr;

AudioDecodeThread::~AudioDecodeThread()
{
    
}

AudioDecodeThread::AudioDecodeThread()
{
    needStop = 0;
}

bool AudioDecodeThread::init(PlayerContext *playerContext)
{
    pPlayerContext = playerContext;
    if(pPlayerContext->audioPacketQueueFunc == NULL)
    {
        printf("pPlayerContext videoPacketQueueFunc is NULL \n");
        return false;
    }
    pPlayerContext->audioPacketQueueFunc->packet_queue_start(&pPlayerContext->audioRingBuffer);
    
    if (FrameQueueFunc::frame_queue_init(&pPlayerContext->audioDecodeRingBuffer, &pPlayerContext->audioRingBuffer, SAMPLE_QUEUE_SIZE, 1) < 0)
    {
        printf("pPlayerContext frame queue init is NULL \n");
        return false;
    }
    
    return true;
    
}

int AudioDecodeThread::get_audio_frame(AVFrame *frame)
{
    int ret = 0;
    
    AVPacket AudioPkt;
    
    ret = pPlayerContext->audioPacketQueueFunc->packet_queue_get(&pPlayerContext->audioRingBuffer, &AudioPkt, 1, &pPlayerContext->audioDecoder->pkt_serial);
    if (ret < 0)
    {
        
        return ret;
    }
    
    // 如果当前读取到的序列号和队列序列号不一样说明当前连续，可能有seek过程导致
    if(pPlayerContext->audioDecoder->pkt_serial != pPlayerContext->audioRingBuffer.serial)
    {
        av_free_packet(&AudioPkt);
        return -1;
    }
    
    if (AudioPkt.stream_index != pPlayerContext->audioStreamIndex)
    {
        av_free_packet(&AudioPkt);
        return -1;
    }
    
    //如果当前这个拿到这个包pkt序列和queue里的packet序列不同，则代表过时的packet,
    if(AudioPkt.data == pPlayerContext->audio_flush_pkt->data)         // 这个video flush pkt 目前我时直接指向demuxer线程创建的那个
    {
        // 表明当前这个pkt为flush_pkt，
        // 每当我们seek后，会在packet queue中先插入一个flush_pkt，更新当前serial，开启新的播放序列
        //那么就要复位解码内部的状态，刷新内部的缓冲区。因为有时候一个frame并不是由一个packet解出来的，那么可能当要播放新的序列
        // 信息时，还存有之前的packet包信息，所以要avcodec flush buffers
        avcodec_flush_buffers(pPlayerContext->audioDecoder->codecContext);
        pPlayerContext->audioDecoder->finished = 0;
        pPlayerContext->audioDecoder->next_pts = pPlayerContext->audioDecoder->start_pts;
        pPlayerContext->audioDecoder->next_pts_tb = pPlayerContext->audioDecoder->start_pts_tb;
        av_free_packet(&AudioPkt);
        return -1;
    }
    
    ret = decoder_decode_frame(&AudioPkt, frame);
    
    if (ret < 0)
    {
        av_free_packet(&AudioPkt);
        return ret;
    }
    
    frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(pPlayerContext->ic, pPlayerContext->ic->streams[pPlayerContext->audioStreamIndex], frame);
    
    av_free_packet(&AudioPkt);
    return ret;
}

int AudioDecodeThread::decoder_decode_frame(const AVPacket *AudioPkt, AVFrame *frame)
{
    int ret = mediaCore::getIntanse()->Decode(AudioPkt, frame);
    return ret;
}

void AudioDecodeThread::run()
{
    needStop = 0;
    AVFrame *frame = av_frame_alloc();
    int serial = 0;
    
    while(!needStop)
    {
        if (pPlayerContext == NULL)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        if (pPlayerContext->audioRingBuffer.size < 0 || pPlayerContext->audioRingBuffer.AvPacketList.empty() ||
            pPlayerContext->audioRingBuffer.nb_packets == 0)        // 判断buffer中是否有包
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        if (pPlayerContext->audioRingBuffer.abort_request)          // 判断videoBuffer是否阻止获取信息
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        int ret = get_audio_frame(frame);
        if(ret < 0)
        {
            if (AVERROR_EOF == ret)
            {
                pPlayerContext->audioDecoder->finished = pPlayerContext->audioDecoder->pkt_serial;
                avcodec_flush_buffers(pPlayerContext->audioDecoder->codecContext);  // 冲刷avcodec信息
                needStop = true;  //说明读取到了结束包信息
            }
            continue;
        }
        
        int pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts;
        int64_t pos = frame->pkt_pos;
        int serial =  pPlayerContext->audioDecoder->pkt_serial;
        int duration = av_q2d((AVRational){frame->nb_samples, frame->sample_rate});
       
        queue_audio(frame, pts, duration, pos, serial);

        av_frame_unref(frame);
    }
}

int AudioDecodeThread::queue_audio(AVFrame *src_frame, double pts, double duration, int64_t pos, int serial)
{
    Frame *ap;
    
    // 从audioDecodeRingBuffer中获取一块Frame大小的可写内存，如果当前的size = max_size的话，说明写满了则返回为空
    if (!(ap = FrameQueueFunc::frame_queue_peek_writable(&pPlayerContext->audioDecodeRingBuffer)))
        return -1;
    
    ap->sar = src_frame->sample_aspect_ratio;
    ap->uploaded = 0;

    ap->format = src_frame->format;
    
    ap->pts = pts;
    ap->duration = duration;
    ap->pos = pos;
    ap->serial = serial;
    
    // 将src中包含的所有内容移至dst并重置src。
    av_frame_move_ref(ap->frame, src_frame);
    // 主要时将写索引往前移动
    FrameQueueFunc::frame_queue_push(&pPlayerContext->audioDecodeRingBuffer);
    return 0;
}



void AudioDecodeThread::start()
{
    thread audio_decode_thread([this]()-> void {              //此处使用lamda表达式
        run();
    });
    audio_decode_thread.detach();
}

void AudioDecodeThread::stop()
{
    needStop = 1;
}

void AudioDecodeThread::close()
{
    
}

NS_MEDIA_END
