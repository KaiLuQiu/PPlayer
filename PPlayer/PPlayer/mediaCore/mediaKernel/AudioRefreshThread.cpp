//
//  AudioRefreshThread.cpp
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#include "AudioRefreshThread.h"
#include "FrameQueueFunc.h"
#include "mediaCore.h"

NS_MEDIA_BEGIN
SDL_mutex *AudioRefreshThread::mutex = SDL_CreateMutex();      //类的静态指针需要在此初始化
AudioRefreshThread* AudioRefreshThread::p_AudioOut = nullptr;


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
    // 这边一定要设置为主线程运行，否则接下来SDL_Init就会fail
    // 设置期望的音频channel layout、nb_samples、sample_rate等参数
    SDL_SetMainReady();
    SDL_AudioSpec wanted_spec, spec;
    const int next_nb_channels[] = {0, 0, 1, 6, 2, 6, 4, 6};
    const int next_sample_rates[] = {0, 44100, 48000, 96000, 192000};
    int next_sample_rate_idx = FF_ARRAY_ELEMS(next_sample_rates) - 1;
    
    // 这边是判断layout 返回的通道个数是否和传入的相等 wanted_channel_layout各个通道存储顺序,av_get_channel_layout_nb_channels根据通道的layout返回通道的个数
    if (wanted_nb_channels != av_get_channel_layout_nb_channels(wanted_channel_layout)
        || !wanted_channel_layout) {
        wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
        wanted_channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
    }
    wanted_nb_channels = av_get_channel_layout_nb_channels(wanted_channel_layout);
    // 设置通道个数
    wanted_spec.channels = wanted_nb_channels;
    // 设置采样率
    wanted_spec.freq = wanted_sample_rate;
    // 如果通道个数or采样率为0，则有问题
    if (wanted_spec.freq <= 0 || wanted_spec.channels <= 0) {
        av_log(NULL, AV_LOG_ERROR, "Invalid sample rate or channel count!\n");
        return -1;
    }
    while (next_sample_rate_idx && next_sample_rates[next_sample_rate_idx] >= wanted_spec.freq) {
        next_sample_rate_idx--;
    }
    // 设置格式
    wanted_spec.format = AUDIO_S16SYS;
    // 设置静音
    wanted_spec.silence = 0;
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
    
    pPlayerContext->audioInfoTarget.fmt = AV_SAMPLE_FMT_S16;
    pPlayerContext->audioInfoTarget.freq = spec.freq;
    pPlayerContext->audioInfoTarget.channel_layout = wanted_channel_layout;
    pPlayerContext->audioInfoTarget.channels =  spec.channels;
    pPlayerContext->audioInfoTarget.frame_size = av_samples_get_buffer_size(NULL, pPlayerContext->audioInfoTarget.channels, 1, pPlayerContext->audioInfoTarget.fmt, 1);
    pPlayerContext->audioInfoTarget.bytes_per_sec = av_samples_get_buffer_size(NULL, pPlayerContext->audioInfoTarget.channels, pPlayerContext->audioInfoTarget.freq, pPlayerContext->audioInfoTarget.fmt, 1);
    if (pPlayerContext->audioInfoTarget.bytes_per_sec <= 0 || pPlayerContext->audioInfoTarget.frame_size <= 0) {
        av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size failed\n");
        return -1;
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
    PCMBuffer_t *pPCMBuffer = NULL;
    int i;
    
    for (i = 0; i < FRAME_QUEUE_SIZE; i++)
    {
        if (PCMBuffers[i].state != DISP_WAIT && PCMBuffers[i].bufferAddr != NULL)
        {
            pPCMBuffer = &PCMBuffers[i];
            break;
        }
    }
    
    if (i == FRAME_QUEUE_SIZE)
    {
        return NULL;
    }
    
    return pPCMBuffer;
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
        if (!pPlayerContext)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        if (!(pPCMBuffer = GetOneValidPCMBuffer()))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        Frame *pFrame;
        pFrame = FrameQueueFunc::frame_queue_peek(&pPlayerContext->audioDecodeRingBuffer); // 获取下一笔要现实的帧
        if (pFrame == NULL)
        {
            continue;
        }
        
        pPCMBuffer->bufferSize = mediaCore::getIntanse()->audioSwr(pPCMBuffer->bufferAddr, pFrame->frame);
        

        
        FrameQueueFunc::frame_queue_next(&pPlayerContext->videoDecodeRingBuffer);
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
