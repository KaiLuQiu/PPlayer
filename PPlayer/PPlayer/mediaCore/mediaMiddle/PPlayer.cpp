//
//  PPlayer.cpp
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#include "PPlayer.h"
#include "AvSyncClock.h"
NS_MEDIA_BEGIN

PPlayer* PPlayer::p_Player = nullptr;
// 类的静态指针需要在此初始化
SDL_mutex* PPlayer::mutex = SDL_CreateMutex();

PPlayer::PPlayer()
{
    pHandler = NULL;
    pPlayerContext = new (std::nothrow)PlayerContext();
    if (NULL == pPlayerContext) {
        printf("PPlayer: new playerInfo fail!!! \n");
    }
    pPlayerContext->volumeValue = 50.0;
    
    p_VideoOut = new (std::nothrow)VideoRefreshThread();
    if (NULL == p_VideoOut) {
        printf("PPlayer: new VideoRefreshThread fail!!! \n");
    }
    
    p_VideoDecoder = new (std::nothrow)VideoDecodeThread();
    if (NULL == p_VideoDecoder) {
        printf("PPlayer: new VideoDecodeThread fail!!! \n");
    }
    
    p_Demuxer = new (std::nothrow)DemuxThread();
    if (NULL == p_Demuxer) {
        printf("PPlayer: new DemuxThread fail!!! \n");
    }
    
    p_AudioOut = new (std::nothrow)AudioRefreshThread();
    if (NULL == p_AudioOut) {
        printf("PPlayer: new AudioRefreshThread fail!!! \n");
    }
    
    p_AudioDecoder = new (std::nothrow)AudioDecodeThread();
    if (NULL == p_AudioDecoder) {
        printf("PPlayer: new AudioDecodeThread fail!!! \n");
    }
    
    p_MediaCore = new (std::nothrow)mediaCore();
    if (NULL == p_MediaCore) {
        printf("PPlayer: new mediaCore fail!!! \n");
    }
}

PPlayer::~PPlayer()
{
    SAFE_DELETE(pPlayerContext);
    SAFE_DELETE(p_VideoOut);
    SAFE_DELETE(p_VideoDecoder);
    SAFE_DELETE(p_Demuxer);
    SAFE_DELETE(p_AudioOut);
    SAFE_DELETE(p_AudioDecoder);
    SAFE_DELETE(p_MediaCore);
}

void PPlayer::setHandle(EventHandler *handle)
{
    pHandler = handle;
}

// 这边暂时只保留url信息
void PPlayer::setDataSource(std::string url)
{
    pUrl = url;
}

int PPlayer::setView(void *view)
{
    if (NULL == p_VideoOut) {
        printf("PPlayer: p_VideoOut is NULL\n");
        return -1;
    }
    p_VideoOut->setView(view);
    return 1;
}

bool PPlayer::prepareAsync()
{
    if (NULL == pHandler && NULL == pPlayerContext) {
        printf("PPlayer: prepareAsync error pHandler or pPlayerContext is NULL!!!\n");
        return false;
    }
    if (NULL == p_MediaCore || NULL == p_Demuxer || NULL == p_VideoDecoder ||
        NULL == p_VideoOut || NULL == p_AudioOut || NULL == p_AudioDecoder) {
        printf("PPlayer: p_Core or p_Demuxer or p_VideoDecoder or p_VideoOut or p_AudioOut or p_AudioDecoder is NULL\n");
        return false;
    }
    p_MediaCore->Init(pPlayerContext, pHandler);
    // avformat和avcodec都打开了
    bool ret = p_MediaCore->StreamOpen(pUrl);
    if(ret == true)
    {
        p_Demuxer->init(pPlayerContext, pHandler, p_MediaCore);
        // 初始化videodecoder，主要是startPacketQueue
        p_VideoDecoder->init(pPlayerContext, pHandler, p_MediaCore);
        p_VideoOut->init(pPlayerContext, pHandler, p_MediaCore);
        p_AudioOut->init(pPlayerContext, pHandler, p_MediaCore);
        // 初始化videodecoder，主要是startPacketQueue
        p_AudioDecoder->init(pPlayerContext, pHandler, p_MediaCore);
        // 开启demuxer线程读取数据包
        p_Demuxer->start();
        // videoDecode和audioDecode可以在prepareAsync的时候就开启，当显示线程则不可。为了加快第一帧的show
        p_VideoDecoder->start();
        p_AudioDecoder->start();
    }
    // 这边一般要render第一帧之后才能上发prepared消息
    pHandler->sendOnPrepared();
    return true;
}

void PPlayer::prepare()
{
    
}

bool PPlayer::start()
{
    if (NULL == p_VideoOut || NULL == p_AudioOut) {
        printf("PPlayer:p_VideoOut or p_AudioOut is NULL\n");
        return false;
    }
    p_VideoOut->start();
    p_AudioOut->start();
    pHandler->sendOnStart();
    return true;
}

bool PPlayer::pause(bool state)
{
    if (NULL == p_VideoOut || NULL == p_AudioOut || NULL == p_Demuxer) {
        printf("PPlayer:p_VideoOut or p_AudioOut or p_Demuxer is NULL\n");
        return false;
    }
    msgInfo msg;
    // 暂停播放
    if(true == state) {
        msg.cmd = MESSAGE_CMD_PAUSE;
        msg.data = -1;
        p_VideoOut->queueMessage(msg);
        p_AudioOut->queueMessage(msg);
        p_Demuxer->queueMessage(msg);
        pPlayerContext->AudioClock.paused = 1;
        pPlayerContext->VideoClock.paused = 1;
    }
    else {
        msg.cmd = MESSAGE_CMD_START;
        msg.data = -1;
        // 更新frame_timer;  因为暂停过程中系统时间是一直在走的，last_updated是暂停时刻的系统时间
        pPlayerContext->frame_timer += av_gettime_relative() / 1000000.0 - pPlayerContext->VideoClock.last_updated;
        AvSyncClock::set_clock(&pPlayerContext->VideoClock, AvSyncClock::get_clock(&pPlayerContext->VideoClock), pPlayerContext->VideoClock.serial);
        p_VideoOut->queueMessage(msg);
        p_AudioOut->queueMessage(msg);
        p_Demuxer->queueMessage(msg);
        pPlayerContext->AudioClock.paused = 0;
        pPlayerContext->VideoClock.paused = 0;
    }
    
    return true;
}

int PPlayer::seek(float pos)
{
    int ret;
    if (NULL == p_Demuxer) {
        printf("PPlayer: p_Demuxer is NULL\n");
        return -1;
    }
    if (PLAYER_MEDIA_NOP != pPlayerContext->playerState) {
        msgInfo msg;
        msg.cmd = MESSAGE_CMD_SEEK;
        msg.data = pos;
        // seek到当前位置的后一个I frame
        p_Demuxer->queueMessage(msg);
    } else {
        ret = -1;
    }
    return ret;
}

bool PPlayer::resume()
{
    return true;
}

bool PPlayer::stop()
{
    return true;
}

void PPlayer::flush()
{
    
}

bool PPlayer::setLoop(bool loop)
{
    return true;
}

int64_t PPlayer::getCurPos()
{
    PlayerContext* playerInfo = pPlayerContext;
    if(playerInfo == NULL || playerInfo->ic == NULL)
        return 0;

    int64_t start_time = playerInfo->ic->start_time;
    int64_t start_diff = 0;
    
   if (start_time > 0 && start_time != AV_NOPTS_VALUE)
       start_diff = av_rescale(start_time, 1000, AV_TIME_BASE);
   
    int64_t pos = 0;
    double pos_clock = AvSyncClock::get_master_clock(playerInfo);
    if (isnan(pos_clock)) {
        pos = av_rescale(playerInfo->seek_pos, 1000, AV_TIME_BASE);
    } else {
        pos = pos_clock * 1000;
    }

   if (pos < 0 || pos < start_diff)
       return 0;

   int64_t adjust_pos = pos - start_diff;
   return (long)adjust_pos;
}

bool PPlayer::setSpeed()
{
    return true;
}

float PPlayer::getSpeed()
{
    return 0;
}

long PPlayer::getDuration()
{
    PlayerContext* playerInfo = pPlayerContext;

     if (!playerInfo || !playerInfo->ic)
        return 0;
    int64_t duration = av_rescale(playerInfo->ic->duration, 1000, AV_TIME_BASE);
    if (duration < 0)
        return 0;
    
    return (long)duration;
}

void PPlayer::setVolume(float value)
{
    PlayerContext* playerInfo = pPlayerContext;

    if (NULL != playerInfo) {
        pPlayerContext->volumeValue = value;
    }
}

void PPlayer::pp_get_msg(Message& msg)
{
    switch(msg.m_what){
    case PLAYER_MEDIA_NOP:
        break;

    case PLAYER_MEDIA_SEEK:
        break;

    case PLAYER_MEDIA_PREPARED:
            
        pPlayerContext->playerState = PLAYER_MEDIA_PREPARED;
        break;

    case PLAYER_MEDIA_SEEK_COMPLETE:
        // 这边将PCMBuffer中的数据给flush掉
        p_AudioOut->flush();

        break;
    case PLAYER_MEDIA_SEEK_FAIL:
        break;

    case PLAYER_MEDIA_PLAYBACK_COMPLETE:
        break;
            
    case PLAYER_MEDIA_SET_VIDEO_SIZE:
            
        break;
    case PLAYER_MEDIA_ERROR:
            
        break;
            
    case PLAYER_MEDIA_INFO:
            
        break;
    case PLAYER_MEDIA_PAUSE:
        
        pPlayerContext->playerState = PLAYER_MEDIA_PAUSE;
        break;
    case PLAYER_MEDIA_START:
            
        pPlayerContext->playerState = PLAYER_MEDIA_START;
        break;
    default:
        break;
    }
}
NS_MEDIA_END
