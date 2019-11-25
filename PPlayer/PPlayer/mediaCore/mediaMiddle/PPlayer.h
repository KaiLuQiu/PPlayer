//
//  PPlayer.h
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#ifndef PPLAYER_H
#define PPLAYER_H

#include <string>
#include "MediaDefineInfo.h"
#include "mediaCore.h"
#include "Demuxthread.h"
#include "VideoDecodeThread.h"
#include "VideoRefreshThread.h"

NS_MEDIA_BEGIN

class PPlayer
{
public:
    static PPlayer *getInstance() {         //懒汉模式
        if(NULL == p_Player) {
            SDL_LockMutex(mutex);
            if(NULL == p_Player) {
                p_Player = new (std::nothrow)PPlayer();
                if(p_Player == NULL) {
                    printf("PPlayer getInstance is NULL!");
                }
            }
            SDL_UnlockMutex(mutex);
        }
        return p_Player;
    }
    
    void setDataSource(std::string url);
    void prepareAsync();
    void prepare();
    bool start();
    bool pause();
    bool seek(int64_t pos);
    bool resume();
    bool stop();
    void flush();
    bool setLoop(bool loop);
    int getCurPos();
    bool setSpeed();
    float getSpeed();
    int setView(void *view);

    
    PlayerContext* getPlayerContext()      //写在class内相当于内联函数
    {
        return pPlayerContext;
    }
    PPlayer();              
    virtual ~PPlayer();     //析构函数一定不能私有话，否则可能导致内存泄漏
private:
    PlayerContext *pPlayerContext;
    
    static PPlayer *p_Player;
    static SDL_mutex *mutex;
    std::string pUrl;
};

NS_MEDIA_END
#endif // PPLAYER_H
