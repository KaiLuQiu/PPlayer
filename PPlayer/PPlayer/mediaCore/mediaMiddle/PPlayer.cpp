//
//  PPlayer.cpp
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//
#include "PPlayer.h"

PPlayer* PPlayer::p_Player = nullptr;
SDL_mutex* PPlayer::mutex = SDL_CreateMutex();      //类的静态指针需要在此初始化

PPlayer::PPlayer()
{
    
}
PPlayer::~PPlayer()
{
    
}

void PPlayer::setDataSource(std::string url)        //这边暂时只保留url信息
{
    pUrl = url;
}

void PPlayer::prepareAsync()
{
    mediaCore::getIntanse()->StreamOpen(pUrl);
    
}

void PPlayer::prepare()
{
    
}

bool PPlayer::start()
{
    return true;
}

bool PPlayer::pause()
{
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
