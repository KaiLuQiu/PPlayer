//
//  ViewController.m
//  PPlayer
//
//  Created by 孙鹏举 on 2018/10/11.
//  Copyright © 2018年 孙鹏举. All rights reserved.
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
}

-(void) setView:(void *)view
{
    media::PPlayer::getInstance()->setView(view);

}

-(void) start {
    PlayerState curState = media::PPlayer::getInstance()->getPlayerContext()->playerState;
    // prepared状态后，才能正式播放
    if(curState == PLAYER_STATE_PREPARED) {
        media::PPlayer::getInstance()->start();
    }
}

-(void) pause:(Boolean)isPause {
    PlayerState curState = media::PPlayer::getInstance()->getPlayerContext()->playerState;
    if (curState == PLAYER_STATE_START ||
        curState == PLAYER_STATE_SEEK ||
        curState == PLAYER_STATE_RESUME ||
        curState == PLAYER_STATE_FLUSH ||
        curState == PLAYER_STATE_PAUSE) {
        media::PPlayer::getInstance()->pause(isPause);
    }
}

-(void) stop {
    
}

-(float)getCurPos {
    float pos = (float)media::PPlayer::getInstance()->getCurPos() / 1000;
    printf("cur pos %f\n", pos);
    return pos;
}

-(void)setSpeed {
    
}

-(float)getSpeed {
    return 0.0;
}

-(float)getDuration {
   return (float)media::PPlayer::getInstance()->getDuration() / 1000;
}

@end
