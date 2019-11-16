//
//  AyncClock.cpp
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#include "meidaCore.h"
#include "PPlayer.h"
mediaCore* mediaCore::p_Core = nullptr;
SDL_mutex *mediaCore::mutex = SDL_CreateMutex();      //类的静态指针需要在此初始化

mediaCore::mediaCore()
{
    avFormatContext = NULL;
    
    av_register_all();      //挂载demuxer filter muxer decoder等
    avformat_network_init();
}

mediaCore::~mediaCore()
{

}

void mediaCore::Init(PlayerContext *playerContext)
{
//    avFormatContext = p_PlayerContext->ic;
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
    
//    AVDictionary **opts = setup_find_stream_info_opts(p_PlayerContext->ic, codec_opts);
//    int orig_nb_streams = p_PlayerContext->ic->nb_streams;
//
    err = avformat_find_stream_info(p_PlayerContext->ic, NULL);     //去parse流的相关信息
    
//    for (int i = 0; i < orig_nb_streams; i++)
//        av_dict_free(&opts[i]);
//    av_freep(&opts);
    
    if (err < 0) {
        
        printf("avformat_find_stream_info  fail/n");
        return false;
    }
    
    if (p_PlayerContext->ic->pb)
        p_PlayerContext->ic->pb->eof_reached = 0; // FIXME hack, ffplay maybe should not use avio_feof() to test for the end
    
    if (seek_by_bytes < 0) {
        seek_by_bytes = !!(p_PlayerContext->ic->iformat->flags & AVFMT_TS_DISCONT) && strcmp("ogg", p_PlayerContext->ic->iformat->name);
    }
    
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
            OpenVideoDecode(i);
        }
        else if ((p_CodecContex->codec_type == AVMEDIA_TYPE_AUDIO) && (st_index[AVMEDIA_TYPE_AUDIO] >= 0))//音频流
        {
            OpenAudioDecode(i);
        }
    }
    
    return true;
}

bool mediaCore::OpenVideoDecode(int streamIndex)
{
    p_PlayerContext->video_avctx = avcodec_alloc_context3(NULL);
    int ret = avcodec_parameters_to_context(p_PlayerContext->video_avctx, p_PlayerContext->ic->streams[streamIndex]->codecpar);
    
    if(ret < 0)
        return false;
    
    av_codec_set_pkt_timebase(p_PlayerContext->video_avctx, p_PlayerContext->ic->streams[streamIndex]->time_base);
    
    //            p_PlayerContext->video_avctx = p_PlayerContext->ic->streams[i]->codec;
    float fps = r2d(p_PlayerContext->ic->streams[streamIndex]->r_frame_rate);
    AVCodec *codec = avcodec_find_decoder(p_PlayerContext->video_avctx->codec_id);
    if (!codec)
    {
        return false;
    }
    else
    {
        int ret = avcodec_open2(p_PlayerContext->video_avctx, codec, NULL);
        if (ret != 0)
        {
            char buff[1024] = { 0 };
            av_strerror(ret, buff, sizeof(buff));
            return false;
        }
        p_PlayerContext->width = p_PlayerContext->video_avctx->width;
        p_PlayerContext->height = p_PlayerContext->video_avctx->height;
    }
    return true;
}


bool mediaCore::OpenAudioDecode(int streamIndex)
{
    p_PlayerContext->audio_avctx = avcodec_alloc_context3(NULL);
    if (!p_PlayerContext->audio_avctx)
        return false;
    
    int ret = avcodec_parameters_to_context(p_PlayerContext->audio_avctx, p_PlayerContext->ic->streams[streamIndex]->codecpar);
    
    if(ret < 0)
        return false;
    
    AVCodec *codec = avcodec_find_decoder(p_PlayerContext->audio_avctx->codec_id);
    if (!codec)
    {
        return false;
    }
    else
    {
        int ret = avcodec_open2(p_PlayerContext->audio_avctx, codec, NULL);
        if (ret != 0)
        {
            char buff[1024] = { 0 };
            av_strerror(ret, buff, sizeof(buff));
            return false;
        }
        
        p_PlayerContext->audioInfo.freq = p_PlayerContext->audio_avctx->sample_rate;
        p_PlayerContext->audioInfo.channels = p_PlayerContext->audio_avctx->channels;
        p_PlayerContext->audioInfo.frame_size = p_PlayerContext->audio_avctx->frame_size;
        
    }
    return true;
}
