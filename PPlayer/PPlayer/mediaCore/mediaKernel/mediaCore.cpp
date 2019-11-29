//
//  AyncClock.cpp
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#include "mediaCore.h"
#include "PPlayer.h"
NS_MEDIA_BEGIN



mediaCore* mediaCore::p_Core = nullptr;
SDL_mutex *mediaCore::mutex = SDL_CreateMutex();      //类的静态指针需要在此初始化

mediaCore::mediaCore()
{
    avFormatContext = NULL;
    swr_ctx = NULL;
    av_register_all();      // 挂载demuxer filter muxer decoder等
    avformat_network_init();
}

mediaCore::~mediaCore()
{

}

void mediaCore::Init(PlayerContext *playerContext)
{
    p_PlayerContext = playerContext;
}

bool mediaCore::StreamOpen(std::string pUrl)
{
    int seek_by_bytes = -1;
    
    int st_index[AVMEDIA_TYPE_NB]; //流索引数组
    memset(st_index, -1, sizeof(st_index));
    p_PlayerContext->ic = avformat_alloc_context();   //对AvFormat内存空间的申请
    if (!p_PlayerContext->ic)
    {
        printf("avformat alloc fail/n");
        return false;
    }
    //打开AvFormat
    int err = avformat_open_input(&p_PlayerContext->ic, pUrl.c_str(), p_PlayerContext->avformat, NULL);
    if (err < 0) {
        printf(pUrl.c_str(), err);
        return false;
    }
    
    err = avformat_find_stream_info(p_PlayerContext->ic, NULL);     //去parse流的相关信息
    if (err < 0) {
        
        printf("avformat_find_stream_info  fail/n");
        return false;
    }
    
    if (p_PlayerContext->ic->pb)
        p_PlayerContext->ic->pb->eof_reached = 0; // FIXME hack, ffplay maybe should not use avio_feof() to test for the end
    
    if (seek_by_bytes < 0) {
        seek_by_bytes = !!(p_PlayerContext->ic->iformat->flags & AVFMT_TS_DISCONT) && strcmp("ogg", p_PlayerContext->ic->iformat->name);
    }
    
    p_PlayerContext->max_frame_duration = (p_PlayerContext->ic->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;
    
    //dump avformat的相关信息
#ifdef Debug
        av_dump_format(p_PlayerContext->ic, 0, pUrl.c_str(), 0);
#endif
    
    //这边一般是获取第一个流
    st_index[AVMEDIA_TYPE_VIDEO] =
        av_find_best_stream(p_PlayerContext->ic, AVMEDIA_TYPE_VIDEO,
                            st_index[AVMEDIA_TYPE_VIDEO], -1, NULL, 0);
    
    st_index[AVMEDIA_TYPE_AUDIO] =
        av_find_best_stream(p_PlayerContext->ic, AVMEDIA_TYPE_AUDIO,
                            st_index[AVMEDIA_TYPE_AUDIO],
                            st_index[AVMEDIA_TYPE_VIDEO],
                            NULL, 0);
    
    int ret = -1;
    //如果存在视频流的,则打开
    /* open the streams */
    for (unsigned int i = 0; i < p_PlayerContext->ic->nb_streams; i++)
    {
        AVCodecContext *p_CodecContex = p_PlayerContext->ic->streams[i]->codec;
        if ((p_CodecContex->codec_type == AVMEDIA_TYPE_VIDEO) && (st_index[AVMEDIA_TYPE_VIDEO] >= 0))//视频流
        {
            p_PlayerContext->videoDecoder = new DecoderContext();
            OpenVideoDecode(i);
        }
        else if ((p_CodecContex->codec_type == AVMEDIA_TYPE_AUDIO) && (st_index[AVMEDIA_TYPE_AUDIO] >= 0))//音频流
        {
            p_PlayerContext->audioDecoder = new DecoderContext();
            OpenAudioDecode(i);
        }
    }
    
    return true;
}

bool mediaCore::OpenVideoDecode(int streamIndex)
{
    
    p_PlayerContext->videoStreamIndex = streamIndex;
    p_PlayerContext->videoDecoder->codecContext = avcodec_alloc_context3(NULL); //申请AVCodecContext空间,可以选择性传递一个解码器，不传直接NULL
    
    int ret = avcodec_parameters_to_context(p_PlayerContext->videoDecoder->codecContext, p_PlayerContext->ic->streams[streamIndex]->codecpar); //将流的信息直接复制到解码器上
    
    if(ret < 0)
        return false;
    
    av_codec_set_pkt_timebase(p_PlayerContext->videoDecoder->codecContext, p_PlayerContext->ic->streams[streamIndex]->time_base);
    
    float fps = r2d(p_PlayerContext->ic->streams[streamIndex]->r_frame_rate);
    
    AVCodec *codec = avcodec_find_decoder(p_PlayerContext->videoDecoder->codecContext->codec_id);
    if (!codec)
    {
        return false;
    }
    else
    {
        int ret = avcodec_open2(p_PlayerContext->videoDecoder->codecContext, codec, NULL);
        if (ret != 0)
        {
            char buff[1024] = { 0 };
            av_strerror(ret, buff, sizeof(buff));
            return false;
        }
        p_PlayerContext->width = p_PlayerContext->videoDecoder->codecContext->width;
        p_PlayerContext->height = p_PlayerContext->videoDecoder->codecContext->height;
    }
    return true;
}


bool mediaCore::OpenAudioDecode(int streamIndex)
{
    p_PlayerContext->audioStreamIndex = streamIndex;
    p_PlayerContext->audioDecoder->codecContext = avcodec_alloc_context3(NULL);
    if (!p_PlayerContext->audioDecoder->codecContext)
        return false;
    
    int ret = avcodec_parameters_to_context(p_PlayerContext->audioDecoder->codecContext, p_PlayerContext->ic->streams[streamIndex]->codecpar);
    
    if(ret < 0)
        return false;
    
    AVCodec *codec = avcodec_find_decoder(p_PlayerContext->audioDecoder->codecContext->codec_id);
    if (!codec)
    {
        return false;
    }
    else
    {
        int ret = avcodec_open2(p_PlayerContext->audioDecoder->codecContext, codec, NULL);
        if (ret != 0)
        {
            char buff[1024] = { 0 };
            av_strerror(ret, buff, sizeof(buff));
            return false;
        }
        
        p_PlayerContext->audioInfo.freq = p_PlayerContext->audioDecoder->codecContext->sample_rate;
        p_PlayerContext->audioInfo.channels = p_PlayerContext->audioDecoder->codecContext->channels;
        p_PlayerContext->audioInfo.frame_size = p_PlayerContext->audioDecoder->codecContext->frame_size;
        p_PlayerContext->audioInfo.channel_layout = p_PlayerContext->audioDecoder->codecContext->channel_layout;
        p_PlayerContext->audioInfo.sample_rate = p_PlayerContext->audioDecoder->codecContext->sample_rate;

        
    }
    return true;
}

int mediaCore::Decode(const AVPacket *pkt, AVFrame *frame)
{
    DecoderContext *codecContext;
    SDL_LockMutex(mutex);
    
    if (!p_PlayerContext->ic || pkt == NULL || frame == NULL )
    {
        SDL_UnlockMutex(mutex);
        return -1;
    }
    
    if (pkt->stream_index != p_PlayerContext->audioStreamIndex && pkt->stream_index != p_PlayerContext->videoStreamIndex)
    {
        SDL_UnlockMutex(mutex);
        return -1;
    }
    
    if (pkt->stream_index == p_PlayerContext->audioStreamIndex)
        codecContext = p_PlayerContext->audioDecoder;
    else
        codecContext = p_PlayerContext->videoDecoder;
    
    int ret = avcodec_send_packet(codecContext->codecContext, pkt);

    if (ret < 0)
    {
        if(ret == AVERROR(EAGAIN))
        {
            int a= 0;
        }
        if(ret == AVERROR_EOF)
        {
            int b = 0;
        }
        if(ret == AVERROR(EINVAL))
        {
            int c = 0;
        }
//        AVERROR(EAGAIN)：当前不接受输出，必须重新发送
//        AVERROR_EOF：已经刷新×××，没有新的包可以被刷新
//        AVERROR(EINVAL)：没有打开×××，或者这是一个编码器，或者要求刷新
//        AVERRO(ENOMEN)：无法添加包到内部队列
        SDL_UnlockMutex(mutex);
        return ret;
    }
    
    ret = avcodec_receive_frame(codecContext->codecContext, frame);
    if (ret < 0) {
        
        SDL_UnlockMutex(mutex);
        return ret;
    }
    
    AVRational playTimeBase;
    playTimeBase.num = 1;
    playTimeBase.den = 1000;
    frame->pts = av_rescale_q_rnd(frame->pts,p_PlayerContext->ic->streams[pkt->stream_index]->time_base,
                                  p_PlayerContext->ic->streams[pkt->stream_index]->codec->time_base, AV_ROUND_NEAR_INF);
    frame->pts = av_rescale_q_rnd(frame->pts,p_PlayerContext->ic->streams[pkt->stream_index]->codec->time_base,
                                  playTimeBase, AV_ROUND_NEAR_INF);
    
    SDL_UnlockMutex(mutex);
    
    return 1;
}



int mediaCore::ResSampleInit(Frame* pFrame, int64_t dec_channel_layout)
{
    if(swr_ctx == NULL)
    {
        swr_ctx = swr_alloc();
    }
    // 释放之前的重采样对象
    swr_free(&swr_ctx);
    
    int a = p_PlayerContext->audioInfoTarget.channel_layout;
    int b = p_PlayerContext->audioInfoTarget.freq;
    int c = p_PlayerContext->audioInfoTarget.channels;
    int d = pFrame->frame->format;
    int e = pFrame->frame->sample_rate;
    
    // 对音频转上下文设置参数信息
    // 使用pFrame(源)和p_PlayerContext->audioInfoTarget(目标)中的音频参数来设置swr_ctx
    swr_ctx = swr_alloc_set_opts(NULL, p_PlayerContext->audioInfoTarget.channel_layout, p_PlayerContext->audioInfoTarget.fmt,
                       p_PlayerContext->audioInfoTarget.freq, dec_channel_layout, (AVSampleFormat)pFrame->frame->format, pFrame->frame->sample_rate,
                       0, NULL);

    if (!swr_ctx || swr_init(swr_ctx) < 0) {
        av_log(NULL, AV_LOG_ERROR,
               "Cannot create sample rate converter for conversion of %d Hz %s %d channels to %d Hz %s %d channels!\n",
               pFrame->frame->sample_rate, av_get_sample_fmt_name((AVSampleFormat)pFrame->frame->format), pFrame->frame->channels,
               p_PlayerContext->audioInfoTarget.freq, av_get_sample_fmt_name(p_PlayerContext->audioInfoTarget.fmt), p_PlayerContext->audioInfoTarget.channels);
        swr_free(&swr_ctx);
        return -1;
    }
    
    // 跟新audioInfo的信息
    // 使用pFrame中的参数更新p_PlayerContext->audioInfo，第一次更新后后面基本不用执行此if分支了，因为一个音频流中各frame通用参数一样
    p_PlayerContext->audioInfo.channel_layout = dec_channel_layout;
    p_PlayerContext->audioInfo.channels       = pFrame->frame->channels;
    p_PlayerContext->audioInfo.freq = pFrame->frame->sample_rate;
    p_PlayerContext->audioInfo.fmt = (AVSampleFormat)pFrame->frame->format;
    return 1;
}

SwrContext* mediaCore::getSwrContext()
{
    return swr_ctx;
}

int mediaCore::audioResample(char *out, int out_samples, AVFrame* frame)
{
    int resampled_data_size;
    
    if (!p_PlayerContext->ic || !frame || !out)
    {
        return 0;
    }
    
    const uint8_t **in = (const uint8_t **)frame->extended_data;
    
    uint8_t *data[1];
    data[0] = (uint8_t *)out;
    // 音频重采样：返回值是重采样后得到的音频数据中单个声道的样本数
    int len = swr_convert(swr_ctx, data, out_samples, in, frame->nb_samples);
    if (len < 0)
    {
        return 0;
    }
    
    // 重采样返回的一帧音频数据大小(以字节为单位)
    resampled_data_size = len * p_PlayerContext->audioInfoTarget.channels * av_get_bytes_per_sample(p_PlayerContext->audioInfoTarget.fmt);
    return resampled_data_size;
    
}
NS_MEDIA_END
