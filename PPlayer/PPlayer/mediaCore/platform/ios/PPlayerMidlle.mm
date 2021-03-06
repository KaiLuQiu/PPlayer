//
//  ViewController.m
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019年 邱开禄. All rights reserved.
//

#import "PPlayerMidlle.h"
#include "PPlayer.h"
#include "PPlayer_C_Interface.h"
#include "EventHandler.h"
#include "Message.h"
@interface PPlayerMidlle ()
    @property media::EventHandler *pHandler;
    @property media::msg_loop pMsgLoopCallback;
    -(int) media_player_msg_loop:(media::Message &)msg;
@end

@implementation PPlayerMidlle

-(id) initPlayer:(const char*)URL {
    if(!(self = [super init])) {
        return nil;
    }
    media::PPlayer::getInstance()->setDataSource(URL);
    
    self.pMsgLoopCallback = msg_loop;
    
    self.pHandler = new (std::nothrow)media::EventHandler();
    if (NULL == self.pHandler) {
        printf("new handler fail!!! \n");
    }
    self.pHandler->setMediaPlayer((__bridge void*)self, self.pMsgLoopCallback);
    media::PPlayer::getInstance()->setHandle(self.pHandler);
    return self;
}

-(void) prepareAsync {
    media::PPlayer::getInstance()->prepareAsync();
}

-(void) setView:(void *)view
{
    media::PPlayer::getInstance()->setView(view);

}

-(void) start {
    PlayerState curState = media::PPlayer::getInstance()->getPlayerContext()->playerState;
    // prepared状态后，才能正式播放
    if(curState == PLAYER_MEDIA_PREPARED) {
        media::PPlayer::getInstance()->start();
    }
}

-(void) pause:(Boolean)isPause {
    PlayerState curState = media::PPlayer::getInstance()->getPlayerContext()->playerState;
    if (curState == PLAYER_MEDIA_START ||
        curState == PLAYER_MEDIA_SEEK ||
        curState == PLAYER_MEDIA_RESUME ||
        curState == PLAYER_MEDIA_FLUSH ||
        curState == PLAYER_MEDIA_PAUSE) {
        media::PPlayer::getInstance()->pause(isPause);
    }
}

-(void) stop {
    
}

-(void) seek:(float)pos {
    media::PPlayer::getInstance()->seek(pos);
}


-(int64_t)getCurPos {
    int64_t pos = media::PPlayer::getInstance()->getCurPos() / 1000;
    return pos;
}

-(void)setSpeed {
    
}

-(float)getSpeed {
    return 0.0;
}

-(int64_t)getDuration {
   return media::PPlayer::getInstance()->getDuration() / 1000;
}

-(void)setVolume:(float)value {
    media::PPlayer::getInstance()->setVolume(value);
}

-(void)dealloc {
    SAFE_DELETE(self.pHandler);
}


void msg_loop(void* playerInstance, media::Message & msg)
{
    // 通过将self指针桥接为oc 对象来调用oc方法
    [(__bridge id)playerInstance media_player_msg_loop:msg];
}

-(int) media_player_msg_loop:(media::Message &)msg
{
    media::PPlayer::getInstance()->pp_get_msg(msg);
    switch(msg.m_what)
    {
        case PLAYER_MEDIA_NOP:
            break;

        case PLAYER_MEDIA_SEEK:
            break;

        case PLAYER_MEDIA_PREPARED:
            if (self.pPreparedListener)
                [self.pPreparedListener onPrepared];
            break;

        case PLAYER_MEDIA_SEEK_COMPLETE:
            if (self.pSeekCompletionListener)
                [self.pSeekCompletionListener onCompletion];
            break;

        case PLAYER_MEDIA_SEEK_FAIL:
            break;

        case PLAYER_MEDIA_PLAYBACK_COMPLETE:
            if (self.pCompletionListener)
                [self.pCompletionListener onCompletion];
            break;

        case PLAYER_MEDIA_SET_VIDEO_SIZE:

            break;
        case PLAYER_MEDIA_ERROR:
            if (self.pErrorListener)
                [self.pErrorListener onCompletion];
            break;

        case PLAYER_MEDIA_INFO:
            if (self.pInfoListener)
                [self.pInfoListener onCompletion];
            break;

        case PLAYER_MEDIA_PAUSE:

            break;
        case PLAYER_MEDIA_START:

            break;
        default:
            break;
    }
    return 0;
}

-(void)setOnPreparedListener:(id<OnPreparedListener>)listener {
    self.pPreparedListener = listener;
}

-(void)setOnCompletionListener:(id<OnCompletionListener>)listener {
    self.pCompletionListener = listener;
}

-(void)setOnSeekCompletionListener:(id<OnSeekCompletionListener>)listener {
    self.pSeekCompletionListener = listener;
}

-(void)setOnErrorListener:(id<OnErrorListener>)listener {
    self.pErrorListener = listener;
}

-(void)setOnInfoListener:(id<OnInfoListener>)listener {
    self.pInfoListener = listener;
}


@end
