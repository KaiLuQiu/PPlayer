//
//  ViewController.h
//  PPlayer
//
//  Created by 孙鹏举 on 2018/10/11.
//  Copyright © 2018年 孙鹏举. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface PPlayerMidlle : NSObject

-(id) initPlayer:(const char*)URL;

-(void)prepareAsync;

-(void)setView:(void *)view;

-(void)start;

-(void)pause:(Boolean)isPause;

-(void)stop;

-(float)getCurPos;

-(void)setSpeed;

-(float)getSpeed;

-(float)getDuration;
@end

