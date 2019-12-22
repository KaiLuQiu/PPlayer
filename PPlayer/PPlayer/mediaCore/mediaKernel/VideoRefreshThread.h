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
#include "EventHandler.h"
#include <thread>
#include "mediaCore.h"

extern "C" {
#include "render_frame.h"
}
NS_MEDIA_BEGIN

// videoRefresh的状态
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
    VideoRefreshThread();
    
    virtual ~VideoRefreshThread();
    
    /*
     * Video输出线程的初始化过程
     */
    bool init(PlayerContext *playerContext, EventHandler *handler, mediaCore *p_Core);
    
    /*
     * Video输出线程的初始化过程
     */
    void start();
    
    /*
     * Video输出线程的主要运行代码
     */
    void run();
    
    /*
     * 停止输出
     */
    void stop();
    
    /*
     * 是否需要AVSync
     */
    int NeedAVSync();
    
    /*
     * 更新video pts时间
     */
    void update_video_pts(double pts, int64_t pos, int serial);
    /*
     * 计算当前现实这笔的pts和下一笔的pts的差值
     */
    double vp_duration(Frame *vp, Frame *nextvp);
    
    /*
     * 参考audio clock计算上一帧真正的持续时长
     */
    double compute_target_delay(double delay);
    /*
     * 是否要保留这一帧
     */
    int DecideKeepFrame(int need_av_sync, int64_t pts);
    
    /*
     * 计算late值
     */
    int64_t CalcSyncLate(int64_t pts);
    
    /*
     * 渲染过程
     */
    void video_image_display();
    
    /*
     * cp frame中yuv数据
     */
    void copyYUVFrameData(uint8_t *src, uint8_t *dst, int linesize, int width, int height);
    
    /*
     * 设置viewb以便渲染
     */
    void setView(void *view);

    /*
     * 将msg指令入队列
     */
    bool queueMessage(msgInfo msg);

private:
    void *glView;
    bool bVideoFreeRun;                   // 不进行avsync，让video自由播放
    message *pMessageQueue;               // 当前的message信息
    msgInfo pCurMessage;               // 当前的播放状态
    bool needStop;
    int framedrop;
    bool pPause;                          // 当前是否是pause状态
    PlayerContext   *pPlayerContext;
    EventHandler    *pHandler;
    mediaCore       *pMediaCore;

};


NS_MEDIA_END
#endif // VideoRefreshThread_H
