//
//  AudioRefreshThread.cpp
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#include "AudioRefreshThread.h"
NS_MEDIA_BEGIN
SDL_mutex *AudioRefreshThread::mutex = SDL_CreateMutex();      //类的静态指针需要在此初始化
AudioRefreshThread* AudioRefreshThread::p_AudioOut = nullptr;

#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

std::list<PCMBuffer_t *>audioPCMDisp;


void AudioRefreshThread::init(PlayerContext *pPlayer)
{
    

}

void _SDL2_fill_audio_callback(void *udata, unsigned char *stream, int len)
{

}

void AudioRefreshThread::start()
{
    thread audio_refresh_thread([this]()-> void {
        run();
    });
    audio_refresh_thread.detach();
}


PCMBuffer_t *AudioRefreshThread::GetOneValidPCMBuffer()
{
    return NULL;
}

void AudioRefreshThread::stop()
{
    needStop = 1;
    bFirstFrame = 1;
}

void AudioRefreshThread::flush()
{

}

void AudioRefreshThread::run()
{
    Frame *pFrame = NULL;
    PCMBuffer_t *pPCMBuffer = NULL;
    
    needStop = 0;
    while(!needStop)
    {
        
    }
}

AudioRefreshThread::AudioRefreshThread()
{
    pPlayerContext = NULL;
    bFirstFrame = 1;
    needStop = 0;

}

void AudioRefreshThread::deinit()
{
    
}

NS_MEDIA_END
