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
    PPlayer::getInstance()->setDataSource(URL);
    return self;
}

-(void) prepareAsync {
    
    PPlayer::getInstance()->prepareAsync();
}

-(void) start {
    
}

-(void) pause {
    
}

-(void) stop {
    
}

-(int64_t)getCurPos {
    return 0;
}

-(void)setSpeed {
    
}

-(float)getSpeed {
    return 0.0;
}

@end
