//
//  mediaCore.h
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#ifndef mediaCore_H
#define mediaCore_H
#include "MediaDefineInfo.h"
#include <string>
NS_MEDIA_BEGIN

#define Debug 0
class mediaCore {
public:
    /*
     * mediaCore单例
     */
    static mediaCore* getIntanse() {
        if(NULL == p_Core) {
            SDL_LockMutex(mutex);
            if(NULL == p_Core) {
                p_Core = new (std::nothrow)mediaCore();
                if(p_Core == NULL) {
                    printf("mediaCore getInstance is NULL!\n");
                }
            }
            SDL_UnlockMutex(mutex);
        }
        return p_Core;
    }
    
    /*
     * mediaCore单例
     */
    static double r2d(AVRational r)
    {
        return r.num == 0 || r.den == 0 ? 0. : (double)r.num / (double)r.den;
    }
    
    /*
     * mediaCore初始化
     */
    void Init(PlayerContext *p_PlayerContext);
    
    /*
     * 根据url打开流信息
     */
    bool StreamOpen(std::string pUrl);
    
    /*
     * 输入pkt，解码输出frame
     */
    int Decode(const AVPacket *pkt, AVFrame *frame);
    
    /*
     * 输入要seek的点（0～100），设置seek的type, 进行seek过程
     */
    int Seek(float pos, int type);
    
    /*
     * 打开audio输出
     */
    int audio_open();
    
    /*
     * audio重采样输出
     */
    int ResSampleInit(Frame* pFrame, int64_t dec_channel_layout);
    
    /*
     * 获取audio重采样对象
     */
    SwrContext* getSwrContext();
    
    /*
     * audio frame重采样
     */
    int audioResample(uint8_t **out, int out_samples, AVFrame* frame);

    /*
     * 初始化，挂载demuxer filter muxer decoder等
     */
    mediaCore();
    virtual ~mediaCore();
private:
    /*
     * 开启video解码器
     */
    bool OpenVideoDecode(int streamIndex);
    
    /*
     * 开启audio解码器
     */
    bool OpenAudioDecode(int streamIndex);
    static mediaCore *p_Core;
    static SDL_mutex *mutex;
    AVFormatContext *avFormatContext;
    PlayerContext *p_PlayerContext;
    AVDictionary  *codec_opts;
    // 音视频转码上下文
    SwrContext *swr_ctx;
};

NS_MEDIA_END
#endif // meidaCore.h
