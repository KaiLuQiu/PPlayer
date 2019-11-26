//
//  VedioRefreshThread.h
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#ifndef VideoRefreshThread_H
#define VideoRefreshThread_H
#include "MediaDefineInfo.h"
#include <thread>
extern "C" {
#include "render_frame.h"
}
NS_MEDIA_BEGIN

//videoRefresh的状态
enum FrameState{
    FRAME_NEED_NEXT,        // 表示可以获取下一帧
    FRAME_NEED_WAIT,        // 表示需要avsync 此帧需要等待
    FRAME_NEED_SHOW,        // avsync的时间满足，此帧可以展示
    FRAME_NEED_DROP,        // avsync落后，此帧需要drop掉
    FRAME_NEED_FREE         // 此帧进入释放状态
};

class VideoRefreshThread : public std::thread
{
public:
    static VideoRefreshThread *getIntanse()  // 饿汉模式
    {
        if(NULL == p_VideoOut) {
            SDL_LockMutex(mutex);
            if(NULL == p_VideoOut) {
                p_VideoOut = new (std::nothrow)VideoRefreshThread();
                if(p_VideoOut == NULL) {
                    printf("VideoDecodeThread getInstance is NULL!");
                }
            }
            SDL_UnlockMutex(mutex);
        }
        return p_VideoOut;
    }
    void init(PlayerContext *playerContext);
    void start();
    void run();
    void stop();
    int NeedAVSync();
    double vp_duration(Frame *vp, Frame *nextvp);
    int DecideKeepFrame(int need_av_sync, int64_t pts);
    int64_t CalcSyncLate(int64_t pts);
    double compute_target_delay(double delay);
    
    void video_image_display(Frame *vp);
    void copyYUVFrameData(uint8_t *src, uint8_t *dst, int linesize, int width, int height);
    void setView(void *view);

    VideoRefreshThread();
    virtual ~VideoRefreshThread();
private:
    void *glView;
    bool bVideoFreeRun;  // 不进行avsync，让video自由播放
    PlayerContext *pPlayerContext;
    bool needStop;
    static VideoRefreshThread* p_VideoOut;
    static SDL_mutex *mutex;
};


NS_MEDIA_END
#endif // VideoRefreshThread_H
