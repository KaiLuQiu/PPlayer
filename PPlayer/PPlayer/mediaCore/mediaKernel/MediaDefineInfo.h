//
//  MediaDefineInfo.h
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//
#ifndef MediaDefineInfo_H
#define MediaDefineInfo_H
#include "MediaCommon.h"
#include "PacketQueueFunc.h"

NS_MEDIA_BEGIN


typedef struct PlayerContext_T {
    PlayerContext_T()
    {
        avformat = NULL;
        seek_request = -1;
        seek_flags = -1;
        seek_pos = -1;
        seek_rel = -1;
        ic = NULL;
        audio_hw_buf_size = -1;
        keep_last = -1;                   //是否保存最后一帧
        width = -1;
        height = -1;
        videoPacketQueueFunc = NULL;
        audioPacketQueueFunc = NULL;
        videoDecoder = NULL;
        audioDecoder = NULL;
        eof = 0;
    }
    ~PlayerContext_T()
    {
        SAFE_DELETE(avformat);
        seek_request = -1;
        seek_flags = -1;
        seek_pos = -1;
        seek_rel = -1;
        SAFE_DELETE(ic);
        audio_hw_buf_size = -1;
        keep_last = -1;                   //是否保存最后一帧
        width = -1;
        height = -1;
        SAFE_DELETE(videoPacketQueueFunc);
        SAFE_DELETE(audioPacketQueueFunc);
        SAFE_DELETE(videoDecoder);
        SAFE_DELETE(audioDecoder);
        eof = 0;
    }
    AVInputFormat *avformat;        //
    int seek_request;               // 标识一次SEEK请求
    int seek_flags;                 // SEEK标志，诸如AVSEEK_FLAG_BYTE等
    int64_t seek_pos;               // SEEK的目标位置(当前位置+增量)
    int64_t seek_rel;               // 本次SEEK的位置增量
    AVFormatContext *ic;
    PacketQueue videoRingBuffer;    // 存储demuxer出来的未解码的序列帧
    PacketQueue audioRingBuffer;    // 存储demuxer出来的未解码的序列帧
    PacketQueue subtilteRingBuffer; // 存储demuxer出来的未解码的序列帧
    
    FrameQueue videoDecodeRingBuffer; // 存储decode出来的未解码的序列帧
    FrameQueue audioDecodeRingBuffer; // 存储decode出来的未解码的序列帧
    FrameQueue subDecodeRingBuffer;   // 存储decode出来的未解码的序列帧
    
    AudioInfo audioInfo;   // SDL支持的音频参数，重采样转换;
    int audio_hw_buf_size;          // SDL音频缓冲区大小(单位字节)
    bool keep_last;                   //是否保存最后一帧
    int width;                      
    int height;
    int videoStreamIndex;
    int audioStreamIndex;
    int eof;                    //是否parse到类eof标识位
    
    DecoderContext *videoDecoder;       //
    DecoderContext *audioDecoder;       //
    
    PacketQueueFunc *videoPacketQueueFunc;
    PacketQueueFunc *audioPacketQueueFunc;

}PlayerContext;



NS_MEDIA_END
#endif // MediaDefineInfo_H
