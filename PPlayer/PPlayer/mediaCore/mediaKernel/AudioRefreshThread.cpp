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

/* Minimum SDL audio buffer size, in samples. */
#define SDL_AUDIO_MIN_BUFFER_SIZE 512
/* Calculate actual buffer size keeping in mind not cause too frequent audio callbacks */
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30

#define MAX_AUDIO_FRAME_SIZE 192000

std::list<PCMBuffer_t *>audioPCMDisp;

void AudioRefreshThread::audio_callback(void *udata, unsigned char *stream, int len)
{
    int a = 0;
}

int AudioRefreshThread::init(PlayerContext *pPlayer)
{
    if (NULL == pPlayer)
        return -1;
    pPlayerContext = pPlayer;
    
    int64_t wanted_channel_layout = pPlayerContext->audioInfo.channel_layout ;
    int wanted_nb_channels = pPlayerContext->audioInfo.channels ;
    int wanted_sample_rate = pPlayerContext->audioInfo.sample_rate;

    SDL_SetMainReady();                     // 这边一定要设置为主线程运行，否则接下来SDL_Init就会fail
    SDL_AudioSpec wanted_spec, spec;        // 设置期望的音频channel layout、nb_samples、sample_rate等参数
    const int next_nb_channels[] = {0, 0, 1, 6, 2, 6, 4, 6};
    const int next_sample_rates[] = {0, 44100, 48000, 96000, 192000};
    int next_sample_rate_idx = FF_ARRAY_ELEMS(next_sample_rates) - 1;
    
    if (wanted_nb_channels != av_get_channel_layout_nb_channels(wanted_channel_layout)  // 这边是判断layout 返回的通道个数是否和传入的相等 wanted_channel_layout各个通道存储顺序,av_get_channel_layout_nb_channels根据通道的layout返回通道的个数
        || !wanted_channel_layout) {
        wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
        wanted_channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
    }
    wanted_nb_channels = av_get_channel_layout_nb_channels(wanted_channel_layout);
    wanted_spec.channels = wanted_nb_channels;          // 设置通道个数
    wanted_spec.freq = wanted_sample_rate;              // 设置采样率
    if (wanted_spec.freq <= 0 || wanted_spec.channels <= 0) {       // 如果通道个数or采样率为0，则有问题
        av_log(NULL, AV_LOG_ERROR, "Invalid sample rate or channel count!\n");
        return -1;
    }
    while (next_sample_rate_idx && next_sample_rates[next_sample_rate_idx] >= wanted_spec.freq) {
        next_sample_rate_idx--;
    }
    
    wanted_spec.format = AUDIO_S16SYS;      // 设置格式
    wanted_spec.silence = 0;                // 设置静音
    wanted_spec.samples = FFMAX(SDL_AUDIO_MIN_BUFFER_SIZE, 2 << av_log2(wanted_spec.freq / SDL_AUDIO_MAX_CALLBACKS_PER_SEC));
    wanted_spec.callback = audio_callback;
    wanted_spec.userdata =  (void *)pPlayerContext;

    // 打开audio 设备
    while (SDL_OpenAudio(&wanted_spec, &spec) < 0) {
        av_log(NULL, AV_LOG_WARNING, "SDL_OpenAudio (%d channels, %d Hz): %s\n",
               wanted_spec.channels, wanted_spec.freq, SDL_GetError());
        wanted_spec.channels = next_nb_channels[FFMIN(7, wanted_spec.channels)];
        if (!wanted_spec.channels) {
            wanted_spec.freq = next_sample_rates[next_sample_rate_idx--];
            wanted_spec.channels = wanted_nb_channels;
            if (!wanted_spec.freq) {
                av_log(NULL, AV_LOG_ERROR,
                       "No more combinations to try, audio open failed\n");
                return -1;
            }
        }
        wanted_channel_layout = av_get_default_channel_layout(wanted_spec.channels);
    }
    
    if (spec.format != AUDIO_S16SYS) {
        av_log(NULL, AV_LOG_ERROR,
               "SDL advised audio format %d is not supported!\n", spec.format);
        return -1;
    }
    
    if (spec.channels != wanted_spec.channels) {
        wanted_channel_layout = av_get_default_channel_layout(spec.channels);
        if (!wanted_channel_layout) {
            av_log(NULL, AV_LOG_ERROR,
                   "SDL advised channel count %d is not supported!\n", spec.channels);
            return -1;
        }
    }
    
    // 播放audio
    SDL_PauseAudio(0);
    
    return spec.size;
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

AudioRefreshThread::~AudioRefreshThread()
{

}


void AudioRefreshThread::deinit()
{
    
}

NS_MEDIA_END
