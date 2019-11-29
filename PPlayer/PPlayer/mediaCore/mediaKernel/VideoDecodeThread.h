//
//  VideoDecodeThread.h
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#ifndef VideoDecodeThread_H
#define VideoDecodeThread_H
#include "MediaDefineInfo.h"
#include <thread>

NS_MEDIA_BEGIN
class VideoDecodeThread : public std::thread
{
public:
    /*
     * Video解码线程的单例模式：饿汉模式
     */
    static VideoDecodeThread* getIntanse() {
        if(NULL == p_Decoder) {
            SDL_LockMutex(mutex);
            if(NULL == p_Decoder) {
                p_Decoder = new (std::nothrow)VideoDecodeThread();
                if(p_Decoder == NULL) {
                    printf("VideoDecodeThread getInstance is NULL!");
                }
            }
            SDL_UnlockMutex(mutex);
        }
        return p_Decoder;
    }
    
    /*
     * Video解码线程的初始化过程
     */
    bool init(PlayerContext *playerContext);
    
    /*
     * Video解码线程主要运行代码
     */
    void run();
    
    /*
     * 启动Video解码
     */
    void start();
    
    /*
     * 结束Video解码
     */
    void stop();
    
    /*
     * 获取解码后的帧
     */
    int get_video_frame(AVFrame *frame);
    
    /*
     * 传入pkt，进行解码输出frame
     */
    int decoder_decode_frame(const AVPacket *VideoPkt, AVFrame *frame);
    
    /*
     * 将frame丢进FrameQueue中
     */
    int queue_picture(AVFrame *src_frame, double pts, double duration, int64_t pos, int serial);
    virtual ~VideoDecodeThread();
    VideoDecodeThread();
private:
    PlayerContext *pPlayerContext;
    static VideoDecodeThread* p_Decoder;
    static SDL_mutex *mutex;
    int needStop;
};


NS_MEDIA_END

#endif // VedioDecodeThread_H
