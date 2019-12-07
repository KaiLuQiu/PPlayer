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
#include "math.h"
#include "AvSyncClock.h"


NS_MEDIA_BEGIN
// 类的静态指针需要在此初始化
SDL_mutex *AudioRefreshThread::mutex = SDL_CreateMutex();
AudioRefreshThread* AudioRefreshThread::p_AudioOut = nullptr;

void AudioRefreshThread::audio_callback(void *udata, unsigned char *stream, int len) {
    AudioRefreshThread *pART = (AudioRefreshThread *)udata;
    // 获取当前时间
    int64_t audio_callback_time = av_gettime_relative();
    // 这步一定要有，否则声音会异常
    SDL_memset(stream, 0, len);
    // 表示获取当前这个队列列头的buffer
    SDL_LockMutex(pART->pPCMBufferQueue.mutex);
    if (pART->pPCMBufferQueue.Queue.empty()) {
        SDL_UnlockMutex(pART->pPCMBufferQueue.mutex);
        printf("audio refresh thread: Not enough data!!!\n");
        return;
    }
    // 从队列中获取一下笔可用buffer
    PCMBuffer* pPCMBuffer = pART->pPCMBufferQueue.Queue.front();
    // 如果读取到的为空的话，继续pop下一笔，容错处理
    while (NULL == pPCMBuffer) {
        // 首先将这笔从列头pop掉
        pART->pPCMBufferQueue.Queue.pop_front();
        // 将当前的buffer size index 索引置为0
        pART->buffer_size_index = 0;
        // 将当前的buffer状态设置为已显示
        pPCMBuffer->state = DISP_DONE;
        if (!pART->pPCMBufferQueue.Queue.empty()) {
            pPCMBuffer = pART->pPCMBufferQueue.Queue.front();
        } else {
            break;
        }
    }
    // 如果从队列中读取的pPCMBuffer都为空的话，直接返回吧
    if (NULL == pPCMBuffer) {
        printf("audio refresh thread: pPCMBuffer is null!!!\n");
        return;
    }
    SDL_UnlockMutex(pART->pPCMBufferQueue.mutex);

    int buffer_size_read = 0;
    while (len > 0) {
        // 如果buffer_size_index 大于bufferSize的大小的话，表明当前的这笔数据已经读完，需要读取下一笔数据了
        if (pART->buffer_size_index >= pPCMBuffer->bufferSize) {
            SDL_LockMutex(pART->pPCMBufferQueue.mutex);
            // 首先将这笔从列头pop掉
            pART->pPCMBufferQueue.Queue.pop_front();
            // 将当前的buffer size index 索引置为0
            pART->buffer_size_index = 0;
            // 将当前的buffer状态设置为已显示
            pPCMBuffer->state = DISP_DONE;
            // 如果当前的
            if (pART->pPCMBufferQueue.Queue.empty()) {
                SDL_UnlockMutex(pART->pPCMBufferQueue.mutex);
                // 说明当前已经没有数据了，直接设置pPCMBuffer为空，让其直接返回
                pPCMBuffer = NULL;
                printf("audio refresh thread: Not enough data!!!\n");
                break;
            }
            // 从队列中获取一笔新buffer数据
            pPCMBuffer = pART->pPCMBufferQueue.Queue.front();
            // 如果读取到的为空的话，继续pop下一笔，容错处理
            while (NULL == pPCMBuffer) {
               // 首先将这笔从列头pop掉
               pART->pPCMBufferQueue.Queue.pop_front();
               pPCMBuffer->state = DISP_DONE;
               if (!pART->pPCMBufferQueue.Queue.empty()) {
                   pPCMBuffer = pART->pPCMBufferQueue.Queue.front();
               } else {
                   break;
               }
           }
            SDL_UnlockMutex(pART->pPCMBufferQueue.mutex);
        }
        if (NULL == pPCMBuffer) {
            printf("audio refresh thread: pPCMBuffer is null!!!\n");
            break;
        }

        buffer_size_read = pPCMBuffer->bufferSize - pART->buffer_size_index;
        if (buffer_size_read > len) {
            buffer_size_read = len;
        }
        if (buffer_size_read <= 0) {
            break;
        }

        SDL_MixAudioFormat(stream,  (uint8_t*)pPCMBuffer->bufferAddr + pART->buffer_size_index, AUDIO_S16SYS, buffer_size_read, 60);
        len -= buffer_size_read;
        stream += buffer_size_read;
        pART->buffer_size_index += buffer_size_read;
    }

    // 更新audio clock的时间
    if (!isnan(pART->audio_clock))
    {
        // set_clock_at更新audclk时，audio_clock是当前audio_buf的显示结束时间(pts+duration)，由于audio driver本身会持有一小块缓冲区，典型地，会是两块交替使用，所以有2 * is->audio_hw_buf_size.
        AvSyncClock::set_clock_at(&pART->pPlayerContext->AudioClock, pART->audio_clock - (double)(2 * pART->audio_hw_buf_size) / pART->pPlayerContext->audioInfoTarget.bytes_per_sec, pART->audio_clock_serial, audio_callback_time / 1000000.0);
        printf("avsync: audio refresh thread audio Clock = %f\n", pART->audio_clock);
    }
}


int AudioRefreshThread::init(PlayerContext *pPlayer) {
    if (NULL == pPlayer)
        return -1;
    pPlayerContext = pPlayer;
    buffer_size_index = 0;
    // 初始化为PCMBuffers分配空间
    for (int i = 0; i < FRAME_QUEUE_SIZE; i++) {
        PCMBuffers[i].bufferAddr = NULL;
        PCMBuffers[i].state = DISP_NONE;
        PCMBuffers[i].bufferSize = 0;
        PCMBuffers[i].pts = -1;
    }
    
    int64_t wanted_channel_layout = pPlayerContext->audioInfo.channel_layout;
    int wanted_nb_channels = pPlayerContext->audioInfo.channels;
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
//    wanted_spec.samples = (pPlayerContext->audioInfo.frame_size ? pPlayerContext->audioInfo.frame_size : 1536);//ac-3 Dolby digital:1536
    wanted_spec.samples = FFMAX(SDL_AUDIO_MIN_BUFFER_SIZE, 2 << av_log2(wanted_spec.freq / SDL_AUDIO_MAX_CALLBACKS_PER_SEC));
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
    // 输出设备的缓冲区大小，一般来说audio输出设备中有2两块buffer，进行交替渲染，所以说，在更新audio clock的时候，需要将这两块的buffer信息减去才是当当时最新播放的pts信息
    audio_hw_buf_size = spec.size;
    return spec.size;
}

void AudioRefreshThread::start() {
    thread audio_refresh_thread([this]()-> void {
        run();
    });
    audio_refresh_thread.detach();
}

// 获取一个能够使用的PCMBuffers
PCMBuffer *AudioRefreshThread::GetOneValidPCMBuffer() {
    PCMBuffer *pPCMBuffer = NULL;
    int i;
    
    for (i = 0; i < FRAME_QUEUE_SIZE; i++) {
        if (PCMBuffers[i].state != DISP_WAIT) {
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
    pPCMBufferQueue.Queue.clear();
    for (int i = 0; i < FRAME_QUEUE_SIZE; i++)
    {
        PCMBuffers[i].state = DISP_NONE;
        PCMBuffers[i].bufferSize = 0;
        PCMBuffers[i].pts = -1;
    }
}

void AudioRefreshThread::run() {
    Frame *pFrame = NULL;
    PCMBuffer *pPCMBuffer = NULL;
    int64_t dec_channel_layout;
    int wanted_nb_samples;
    needStop = 0;
    AVRational tb;

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
            // 使用前先将之前的buffer内存给释放了
            SAFE_AV_FREE(pPCMBuffer->bufferAddr);
            // 进行重采样过程
            // 重采样输出参数：输出音频样本数(多加了256个样本)
            int out_count = (int64_t)pFrame->frame->nb_samples * pPlayerContext->audioInfoTarget.freq / pFrame->frame->sample_rate + 256;
            // 计算BufferSize
            int out_size  = av_samples_get_buffer_size(NULL, pPlayerContext->audioInfoTarget.channels, out_count, pPlayerContext->audioInfoTarget.fmt, 0);
            // new一块新的内存地址
            pPCMBuffer->bufferAddr = (uint8_t *)av_malloc(out_size);
            if (pPCMBuffer->bufferAddr == NULL) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                av_frame_unref(pFrame->frame);
                continue;
            }
            // 音频重采样
            pPCMBuffer->bufferSize = mediaCore::getIntanse()->audioResample(&pPCMBuffer->bufferAddr, out_count, pFrame->frame);
            if (pPCMBuffer->bufferSize == 0) {
                av_frame_unref(pFrame->frame);
                continue;
            }
        } else {
            // 使用前先将之前的buffer内存给释放了
            SAFE_AV_FREE(pPCMBuffer->bufferAddr);
            // 计算BufferSize
            // 根据frame中指定的音频参数获取缓冲区的大小
            int data_size = av_samples_get_buffer_size(NULL, pFrame->frame->channels,
                                                       pFrame->frame->nb_samples,
                                                       (AVSampleFormat)pFrame->frame->format, 1);
            // new一块新的内存地址
            pPCMBuffer->bufferAddr = (uint8_t *)av_malloc(data_size);
            if (pPCMBuffer->bufferAddr == NULL) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                av_frame_unref(pFrame->frame);
                continue;
            }
            memcpy(pPCMBuffer->bufferAddr, pFrame->frame->data[0], data_size);
            pPCMBuffer->bufferSize = data_size;
        }
        
        // 更新pcmBuffer的pts时间
        pPCMBuffer->pts = pFrame->frame->pts;
        // 更新audio clock时钟
        if (!isnan(pFrame->pts))
            audio_clock = pFrame->pts + (double) pFrame->frame->nb_samples / pFrame->frame->sample_rate;
        else
            audio_clock = NAN;
        
        // 更新audio clock的序列号
        audio_clock_serial = pFrame->serial;

        pPCMBuffer->pts = pFrame->frame->pts;
        SDL_LockMutex(pPCMBufferQueue.mutex);
        pPCMBuffer->state = DISP_WAIT;
        pPCMBufferQueue.Queue.push_back(pPCMBuffer);
        SDL_UnlockMutex(pPCMBufferQueue.mutex);
        // frame引用计数减1
        av_frame_unref(pFrame->frame);
    }
}

AudioRefreshThread::AudioRefreshThread() {
    pPlayerContext = NULL;
    bFirstFrame = 1;
    needStop = 0;
}

AudioRefreshThread::~AudioRefreshThread() {
    SDL_CloseAudio();//Close SDL
    SDL_Quit();

    pPCMBufferQueue.Queue.clear();
    for (int i = 0; i < FRAME_QUEUE_SIZE; i++)
    {
        SAFE_AV_FREE(PCMBuffers[i].bufferAddr);
        PCMBuffers[i].state = DISP_NONE;
        PCMBuffers[i].bufferSize = 0;
        PCMBuffers[i].pts = -1;
    }
}

void AudioRefreshThread::deinit() {
    
    SDL_CloseAudio();//Close SDL
    SDL_Quit();
    
    pPCMBufferQueue.Queue.clear();
    for (int i = 0; i < FRAME_QUEUE_SIZE; i++)
    {
        SAFE_AV_FREE(PCMBuffers[i].bufferAddr);
        PCMBuffers[i].state = DISP_NONE;
        PCMBuffers[i].bufferSize = 0;
        PCMBuffers[i].pts = -1;
    }
}


NS_MEDIA_END
