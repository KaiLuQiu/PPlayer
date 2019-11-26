//
//  AudioDecodeThread.h
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//
#ifndef AudioDecodeThread_H
#define AudioDecodeThread_H
#include "MediaDefineInfo.h"
#include <thread>
NS_MEDIA_BEGIN

class AudioDecodeThread : public std::thread
{
public:
    static AudioDecodeThread* getIntanse() {
        if(NULL == p_Decoder) {
            SDL_LockMutex(mutex);
            if(NULL == p_Decoder) {
                p_Decoder = new (std::nothrow)AudioDecodeThread();
                if(p_Decoder == NULL) {
                    printf("AudioDecodeThread getInstance is NULL!");
                }
            }
            SDL_UnlockMutex(mutex);
        }
        return p_Decoder;
    }
    bool init(PlayerContext *playerContext);
    void run();
    void start();
    void stop();
    int get_audio_frame(AVFrame *frame);
    int decoder_decode_frame(const AVPacket *AudioPkt, AVFrame *frame);
    int queue_audio(AVFrame *src_frame, double pts, double duration, int64_t pos, int serial);

    virtual ~AudioDecodeThread();
    AudioDecodeThread();
private:
    static AudioDecodeThread *p_Decoder;
    static SDL_mutex *mutex;

    PlayerContext *pPlayerContext;
    int needStop;
};


NS_MEDIA_END
#endif // AudioDecodeThread_H
