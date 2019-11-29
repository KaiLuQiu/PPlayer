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
// 类的静态指针需要在此初始化
SDL_mutex *AudioRefreshThread::mutex = SDL_CreateMutex();
AudioRefreshThread* AudioRefreshThread::p_AudioOut = nullptr;

std::list<PCMBuffer_t *>audioPCMDisp;

void AudioRefreshThread::audio_callback(void *udata, unsigned char *stream, int len) {
    AudioRefreshThread *pADT = (AudioRefreshThread *)udata;
    PCMBuffer_t * pPCMBuffer = NULL;
    SDL_memset(stream, 0, len);
    SDL_LockMutex(pADT->ADispPCMQueue.mutex);

    if (pADT->ADispPCMQueue.Queue->empty()) {
        SDL_UnlockMutex(pADT->ADispPCMQueue.mutex);
        return;
    }
    
    pPCMBuffer = pADT->ADispPCMQueue.Queue->front();
    pADT->ADispPCMQueue.Queue->pop_front();
    SDL_UnlockMutex(pADT->ADispPCMQueue.mutex);


    int buffer_size_read = 0;
    int buffer_size_index = 0;
    while (len > 0) {
        buffer_size_read = pPCMBuffer->bufferSize - buffer_size_index;
        if (buffer_size_read > len) {
            buffer_size_read = len;
        }
        if (buffer_size_read <= 0) {
            break;
        }
    
        SDL_MixAudioFormat(stream, (const Uint8*)pPCMBuffer->bufferAddr + buffer_size_index, AUDIO_S16SYS, buffer_size_read, SDL_MIX_MAXVOLUME);
        len -= buffer_size_read;
        buffer_size_index += buffer_size_read;
    }
    pPCMBuffer->state = DISP_DONE;
}


int AudioRefreshThread::init(PlayerContext *pPlayer) {
    if (NULL == pPlayer)
        return -1;
    pPlayerContext = pPlayer;
    
    ADispPCMQueue.Queue = &audioPCMDisp;
    ADispPCMQueue.size = FRAME_QUEUE_SIZE + 1;
    // 初始化为PCMBuffers分配空间
    for (int i = 0; i < FRAME_QUEUE_SIZE; i++) {
        PCMBuffers[i].bufferAddr = (char *)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);
        PCMBuffers[i].state = DISP_NONE;
        PCMBuffers[i].bufferSize = 0;
        PCMBuffers[i].pts = -1;
    }
    
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
    
    // 从采样率数组中找到第一个不大于传入参数wanted_sample_rate的值
    // 音频采样格式有两大类型：planar和packed，假设一个双声道音频文件，一个左声道采样点记作L，一个右声道采样点记作R，则：
    // planar存储格式：(plane1)LLLLLLLL...LLLL (plane2)RRRRRRRR...RRRR
    // packed存储格式：(plane1)LRLRLRLR...........................LRLR
    // 在这两种采样类型下，又细分多种采样格式，如AV_SAMPLE_FMT_S16、AV_SAMPLE_FMT_S16P等，注意SDL2.0目前不支持planar格式
    // channel_layout是int64_t类型，表示音频声道布局，每bit代表一个特定的声道，参考channel_layout.h中的定义，一目了然
    while (next_sample_rate_idx && next_sample_rates[next_sample_rate_idx] >= wanted_spec.freq) {
        next_sample_rate_idx--;
    }
    // 采样格式：S表带符号，16是采样深度(位深)，SYS表采用系统字节序，这个宏在SDL中定义
    wanted_spec.format = AUDIO_S16SYS;
    // 设置静音
    wanted_spec.silence = 0;
    // SDL声音缓冲区尺寸，单位是单声道采样点尺寸x声道数
    // 一帧frame的大小
    wanted_spec.samples = (pPlayerContext->audioInfo.frame_size ? pPlayerContext->audioInfo.frame_size : 1536);//ac-3 Dolby digital:1536
//    wanted_spec.samples = FFMAX(SDL_AUDIO_MIN_BUFFER_SIZE, 2 << av_log2(wanted_spec.freq / SDL_AUDIO_MAX_CALLBACKS_PER_SEC));
    wanted_spec.callback = audio_callback;
    // 提供给回调函数的参数
    // 打开音频设备并创建音频处理线程。期望的参数是wanted_spec，实际得到的硬件参数是spec
    // 1) SDL提供两种使音频设备取得音频数据方法：
    //    a. push，SDL以特定的频率调用回调函数，在回调函数中取得音频数据
    //    b. pull，用户程序以特定的频率调用SDL_QueueAudio()，向音频设备提供数据。此种情况wanted_spec.callback=NULL
    // 2) 音频设备打开后播放静音，不启动回调，调用SDL_PauseAudio(0)后启动回调，开始正常播放音频
    wanted_spec.userdata =  (void *)this;

    // 打开audio 设备
    while (SDL_OpenAudio(&wanted_spec, &spec) < 0) {
        av_log(NULL, AV_LOG_WARNING, "SDL_OpenAudio (%d channels, %d Hz): %s\n",
               wanted_spec.channels, wanted_spec.freq, SDL_GetError());
        // 如果打开音频设备失败，则尝试用不同的声道数或采样率再试打开音频设备
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
    
    // 检查打开音频设备的实际参数：采样格式
    if (spec.format != AUDIO_S16SYS) {
        av_log(NULL, AV_LOG_ERROR,
               "SDL advised audio format %d is not supported!\n", spec.format);
        return -1;
    }
    
    // 检查打开音频设备的实际参数：声道数
    if (spec.channels != wanted_spec.channels) {
        wanted_channel_layout = av_get_default_channel_layout(spec.channels);
        if (!wanted_channel_layout) {
            av_log(NULL, AV_LOG_ERROR,
                   "SDL advised channel count %d is not supported!\n", spec.channels);
            return -1;
        }
    }
    
    // wanted_spec是期望的参数，spec是实际的参数，wanted_spec和spec都是SDL中的结构。
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



void AudioRefreshThread::start() {
    thread audio_refresh_thread([this]()-> void {
        run();
    });
    audio_refresh_thread.detach();
}

// 获取一个能够使用的PCMBuffers
PCMBuffer_t *AudioRefreshThread::GetOneValidPCMBuffer() {
    PCMBuffer_t *pPCMBuffer = NULL;
    int i;
    
    for (i = 0; i < FRAME_QUEUE_SIZE; i++) {
        if (PCMBuffers[i].state != DISP_WAIT && PCMBuffers[i].bufferAddr != NULL) {
            pPCMBuffer = &PCMBuffers[i];
            break;
        }
    }
    
    if (i == FRAME_QUEUE_SIZE) {
        return NULL;
    }
    
    return pPCMBuffer;
}

// 从FrameBufferQueue中获取一个有效的frame
Frame *AudioRefreshThread::GetOneValidFrame() {
    Frame *pFrame = NULL;

    if (!pPlayerContext) {
        return NULL;
    }
    
    // 这边是为了获取一个连续且帧
    do {
        // 获取当前待显示的帧，如果条件不满足则进入wait状态
        if (!(pFrame = FrameQueueFunc::frame_queue_peek_readable(&pPlayerContext->audioDecodeRingBuffer))) {
            return NULL;
        }
        // 获取了一帧之后，要把当前着帧销毁，读索引要++
        FrameQueueFunc::frame_queue_next(&pPlayerContext->audioDecodeRingBuffer);
    } while (pFrame->serial != pPlayerContext->audioRingBuffer.serial);
    return pFrame;
}


void AudioRefreshThread::stop() {
    needStop = 1;
    bFirstFrame = 1;
}

void AudioRefreshThread::flush() {
    ADispPCMQueue.Queue->clear();
    for (int i = 0; i < FRAME_QUEUE_SIZE; i++) {
        PCMBuffers[i].state = DISP_NONE;
        PCMBuffers[i].bufferSize = 0;
        PCMBuffers[i].pts = -1;
    }
}

void AudioRefreshThread::run() {
    Frame *pFrame = NULL;
    PCMBuffer_t *pPCMBuffer = NULL;
    int data_size = 0;
    int64_t dec_channel_layout;
    int wanted_nb_samples;
    needStop = 0;
    while(!needStop) {
        if (!pPlayerContext) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        
        if (!(pPCMBuffer = GetOneValidPCMBuffer())) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        pFrame = GetOneValidFrame();
        if(NULL == pFrame) {
            continue;
        }
        
        // 根据frame中指定的音频参数获取缓冲区的大小
        data_size = av_samples_get_buffer_size(NULL, pFrame->frame->channels,
                                               pFrame->frame->nb_samples,
                                               (AVSampleFormat)pFrame->frame->format, 1);

        // 计算dec_channel_layout，用于确认是否需要重新初始化重采样
        // 获取声道布局
        dec_channel_layout =
        (pFrame->frame->channel_layout && pFrame->frame->channels == av_get_channel_layout_nb_channels(pFrame->frame->channel_layout)) ?
        pFrame->frame->channel_layout : av_get_default_channel_layout(pFrame->frame->channels);
        
        // 判断是否需要重新初始化重采样
        // pPlayerContext->audioInfo是SDL可接受的音频帧数，是audio_open()中取得的参数
        // pPlayerContext->audioInfo = pPlayerContext->audioInfoTarget
        // 此处表示：如果frame中的音频参数 == pPlayerContext->audioInfo == pPlayerContext->audioInfoTarget，那音频重采样的过程就免了(因此时is->swr_ctr是NULL)
        // 　　　　　否则使用frame(源)和pPlayerContext->audioInfoTarget(目标)中的音频参数来设置swr_ctx，并使用frame中的音频参数来赋值pPlayerContext->audioInfo
        if (pFrame->frame->format        != pPlayerContext->audioInfo.fmt ||
            dec_channel_layout       != pPlayerContext->audioInfo.channel_layout ||
            pFrame->frame->sample_rate   != pPlayerContext->audioInfo.freq) {
            // 根据pFrame的信息更新重采样器
            if(mediaCore::getIntanse()->ResSampleInit(pFrame, dec_channel_layout) < 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
        }
        
        // 判断重采样器是否创建成功
        // 如果初始化了重采样，则对这一帧数据重采样输出
        if (mediaCore::getIntanse()->getSwrContext()) {
            // 进行重采样过程
            // 重采样输出参数：输出音频样本数(多加了256个样本)
            int out_count = (int64_t)pFrame->frame->nb_samples * pPlayerContext->audioInfoTarget.freq / pFrame->frame->sample_rate + 256;
            // 音频重采样
            pPCMBuffer->bufferSize = mediaCore::getIntanse()->audioResample(pPCMBuffer->bufferAddr, out_count, pFrame->frame);
            if (pPCMBuffer->bufferSize == 0) {
                av_frame_unref(pFrame->frame);
                pPCMBuffer->state = DISP_DONE;
                continue;
            }
        }
        
        pPCMBuffer->pts = pFrame->frame->pts;
        SDL_LockMutex(ADispPCMQueue.mutex);
        pPCMBuffer->state = DISP_WAIT;
        ADispPCMQueue.Queue->push_back(pPCMBuffer);
        SDL_UnlockMutex(ADispPCMQueue.mutex);
    }
}

AudioRefreshThread::AudioRefreshThread() {
    pPlayerContext = NULL;
    bFirstFrame = 1;
    needStop = 0;
}

AudioRefreshThread::~AudioRefreshThread() {
    SDL_CloseAudio();//Close SDL
    
    for (int i = 0; i < FRAME_QUEUE_SIZE; i++) {
        if (!PCMBuffers[i].bufferAddr) {
            av_free((void *)PCMBuffers[i].bufferAddr);
            PCMBuffers[i].bufferAddr = NULL;
        }
        PCMBuffers[i].state = DISP_NONE;
        PCMBuffers[i].bufferSize = 0;
        PCMBuffers[i].pts = -1;
    }
}

void AudioRefreshThread::deinit() {
    ADispPCMQueue.Queue->clear();
    for (int i = 0; i < FRAME_QUEUE_SIZE; i++) {
        if (!PCMBuffers[i].bufferAddr) {
            av_free((void *)PCMBuffers[i].bufferAddr);
            PCMBuffers[i].bufferAddr = NULL;
        }
        PCMBuffers[i].state = DISP_NONE;
        PCMBuffers[i].bufferSize = 0;
        PCMBuffers[i].pts = -1;
    }
}


NS_MEDIA_END
