//
//  ViewController.m
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019年 邱开禄. All rights reserved.
//

#import "PPlayerMidlle.h"
#include "PPlayer.h"


@interface PPlayerMidlle ()

@end


@implementation PPlayerMidlle

-(id) initPlayer:(const char*)URL {
    if(!(self = [super init])) {
        return nil;
    }
    media::PPlayer::getInstance()->setDataSource(URL);
    return self;
}

-(void) prepareAsync {
    media::PPlayer::getInstance()->prepareAsync();
    if (self.pPreparedListener)
        [self.pPreparedListener onPrepared];
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
