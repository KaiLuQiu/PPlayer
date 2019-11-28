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
    
    static mediaCore* getIntanse() {
        if(NULL == p_Core) {
            SDL_LockMutex(mutex);
            if(NULL == p_Core) {
                p_Core = new (std::nothrow)mediaCore();
                if(p_Core == NULL) {
                    printf("mediaCore getInstance is NULL!");
                }
            }
            SDL_UnlockMutex(mutex);
        }
        return p_Core;
    }
    static double r2d(AVRational r)
    {
        return r.num == 0 || r.den == 0 ? 0. : (double)r.num / (double)r.den;
    }
    
    void Init(PlayerContext *p_PlayerContext);
    bool StreamOpen(std::string pUrl);
    int Decode(const AVPacket *pkt, AVFrame *frame);
    int audioSwr(char *out, AVFrame* frame);


    mediaCore();
    virtual ~mediaCore();
private:
    bool OpenVideoDecode(int streamIndex);
    bool OpenAudioDecode(int streamIndex);
    static mediaCore *p_Core;
    static SDL_mutex *mutex;
    AVFormatContext *avFormatContext;
    PlayerContext *p_PlayerContext;
    AVDictionary  *codec_opts;
    SwrContext *swr_ctx;  //音视频转码上下文
};

NS_MEDIA_END
#endif // meidaCore.h
