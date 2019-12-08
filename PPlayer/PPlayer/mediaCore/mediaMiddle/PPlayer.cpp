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
    pPlayerContext = new PlayerContext();
}
PPlayer::~PPlayer()
{
    SAFE_DELETE(pPlayerContext);
}

// 这边暂时只保留url信息
void PPlayer::setDataSource(std::string url)
{
    pUrl = url;
}

int PPlayer::setView(void *view)
{
    VideoRefreshThread::getIntanse()->setView(view);
    return 1;
}

void PPlayer::prepareAsync()
{
    mediaCore::getIntanse()->Init(pPlayerContext);
    // avformat和avcodec都打开了
    bool ret = mediaCore::getIntanse()->StreamOpen(pUrl);
    if(ret == true)
    {
        DemuxThread::getIntanse()->init(pPlayerContext);
        // 初始化videodecoder，主要是startPacketQueue
        VideoDecodeThread::getIntanse()->init(pPlayerContext);
        VideoRefreshThread::getIntanse()->init(pPlayerContext);
        AudioRefreshThread::getIntanse()->init(pPlayerContext);
        // 初始化videodecoder，主要是startPacketQueue
        AudioDecodeThread::getIntanse()->init(pPlayerContext);
        // 开启demuxer线程读取数据包
        DemuxThread::getIntanse()->start();
        // videoDecode和audioDecode可以在prepareAsync的时候就开启，当显示线程则不可。为了加快第一帧的show
        VideoDecodeThread::getIntanse()->start();
        AudioDecodeThread::getIntanse()->start();
    }
    // 这边一般要render第一帧之后才能上发prepared消息
    pPlayerContext->playerState = PLAYER_STATE_PREPARED;
}

void PPlayer::prepare()
{
    
}

bool PPlayer::start()
{
    pPlayerContext->playerState = PLAYER_STATE_START;
    VideoRefreshThread::getIntanse()->start();
    AudioRefreshThread::getIntanse()->start();
    return true;
}

bool PPlayer::pause(bool state)
{
    MessageCmd msgInfo;
    // 暂停播放
    if(true == state) {
        msgInfo = MESSAGE_CMD_PAUSE;
        VideoRefreshThread::getIntanse()->queueMessage(msgInfo);
        AudioRefreshThread::getIntanse()->queueMessage(msgInfo);
        DemuxThread::getIntanse()->queueMessage(msgInfo);
        pPlayerContext->AudioClock.paused = 1;
        pPlayerContext->VideoClock.paused = 1;
        pPlayerContext->playerState = PLAYER_STATE_PAUSE;
    }
    else {
        msgInfo = MESSAGE_CMD_NONE;
        // 更新frame_timer;  因为暂停过程中系统时间是一直在走的，last_updated是暂停时刻的系统时间
        pPlayerContext->frame_timer += av_gettime_relative() / 1000000.0 - pPlayerContext->VideoClock.last_updated;
        AvSyncClock::set_clock(&pPlayerContext->VideoClock, AvSyncClock::get_clock(&pPlayerContext->VideoClock), pPlayerContext->VideoClock.serial);
        VideoRefreshThread::getIntanse()->queueMessage(msgInfo);
        AudioRefreshThread::getIntanse()->queueMessage(msgInfo);
        DemuxThread::getIntanse()->queueMessage(msgInfo);
        pPlayerContext->AudioClock.paused = 0;
        pPlayerContext->VideoClock.paused = 0;
        pPlayerContext->playerState = PLAYER_STATE_START;
    }
    return true;
}

bool PPlayer::seek(int64_t pos)
{
    return true;
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

int PPlayer::getCurPos()
{
    return 0;
}

bool PPlayer::setSpeed()
{
    return true;
}

float PPlayer::getSpeed()
{
    return 0;
}

NS_MEDIA_END
