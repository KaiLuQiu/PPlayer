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
#include "AudioDecodeThread.h"
#include "VideoRefreshThread.h"
#include "AudioRefreshThread.h"
#include "EventHandler.h"

NS_MEDIA_BEGIN

class PPlayer
{
public:
    /*
     * PPlayer单例模式：饿汉模式
     */
    static PPlayer *getInstance() {
        if(NULL == p_Player) {
            SDL_LockMutex(p_Mutex);
            if(NULL == p_Player) {
                p_Player = new (std::nothrow)PPlayer();
                if(p_Player == NULL) {
                    printf("PPlayer getInstance is NULL!\n");
                }
            }
            SDL_UnlockMutex(p_Mutex);
        }
        return p_Player;
    }
  
    /*
     * 设置url
     */
    void setDataSource(std::string url);
    
    /*
     * 执行播放器的prepareAsync状态，此过程会开启demuxer, audiodecode videodecode等线程。做好初始化工作。
     */
    bool prepareAsync();

    void prepare();
    
    /*
     * 当收到prepared之后，上层播放器可以设置start状态开始播放视频
     */
    bool start();
    
    /*
     * 进入暂停状态
     */
    bool pause(bool state);
    
    /*
     * 进行seek的过程
     */
    int seek(float pos);
    
    /*
     * 进行resume重新同步过程
     */
    bool resume();

    /*
     * stop播放器
     */
    bool stop();
    
    /*
     * flush当前的播放器，也就是吧demuxer audio video decode等数据都清空掉
     */
    void flush();
    
    /*
     * 设置是否循环播放视频
     */
    bool setLoop(bool loop);
    
    /*
     * 获取当前播放的位置信息
     */
    int64_t getCurPos();
    
    /*
     * 设置播放速度
     */
    bool setSpeed();

    /*
     * 获取当前的播放速度
     */
    float getSpeed();

    /*
     * 获取总时长
     */
    long getDuration();
    
    /*
     * 设置音量
     */
    void setVolume(float value);
    
    /*
     * 设置用于渲染显示的view
     */
    int setView(void *view);

    /*
     * 获取播放器上下文信息，写在class内相当于内联函数
     */
    PlayerContext* getPlayerContext()
    {
        return pPlayerContext;
    }
    PPlayer();
    
    void pp_get_msg(Message& msg);
    
    void setHandle(EventHandler *handle);
    /*
     * 获析构函数一定不能私有话，否则可能导致内存泄漏
     */
    virtual ~PPlayer();
private:
    std::string pUrl;
    static SDL_mutex    *p_Mutex;
    PlayerContext       *pPlayerContext;
    static PPlayer      *p_Player;
    EventHandler        *p_Handler;
    mediaCore           *p_MediaCore;
    VideoRefreshThread  *p_VideoOutThread;
    VideoDecodeThread   *p_VideoDecoderThread;
    DemuxThread         *p_DemuxerThread;
    AudioRefreshThread  *p_AudioOutThread;
    AudioDecodeThread   *p_AudioDecoderThread;
};

NS_MEDIA_END
#endif // PPLAYER_H
