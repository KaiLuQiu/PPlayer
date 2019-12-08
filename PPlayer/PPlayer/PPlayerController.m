//
//  PPlayerController.m
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/15.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#import "PPlayerController.h"
#import <VideoToolbox/VideoToolbox.h>
#import <AVFoundation/AVFoundation.h>
#import <GameController/GameController.h>
#import <CoreMotion/CoreMotion.h>

#import "PPlayerMidlle.h"
#import "OpenGLView.h"
#include "render_frame.h"

@interface PPlayerController () {
    PPlayerMidlle *_player;
    dispatch_source_t _timer;
}

@end

@implementation PPlayerController

- (void)viewWillDisappear:(BOOL)animated{
    [super viewWillDisappear:animated];
    dispatch_source_cancel(_timer);
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
    NSString* path = [self getFileFromMainbundleAbsolutePath:@"video/hiphop.mp4"];
    self.view.backgroundColor = [UIColor whiteColor];
    OpenGLView *playerView = [[OpenGLView alloc] initWithFrame:CGRectMake(0, 100, self.view.frame.size.width, self.view.frame.size.width/16*9)];
    playerView.backgroundColor = [UIColor blackColor];
    [self.view addSubview:playerView];
    
    _pStartButton = [UIButton buttonWithType:UIButtonTypeSystem];
    _pStartButton.frame = CGRectMake(SCREENWIDTH_D40 * 30, SCREENHEIGHT_D40 * 20, SCREENWIDTH_D40 * 10, SCREENHEIGHT_D40 * 2);
    [_pStartButton setTitle:@"开始播放" forState:UIControlStateNormal];
    [_pStartButton setTitleColor:[UIColor colorWithRed:1 green:1 blue:1 alpha:1] forState:(UIControlState)UIControlStateNormal];
    _pStartButton.contentMode = UIViewContentModeCenter;
    _pStartButton.backgroundColor = [UIColor colorWithRed:0 green:1 blue:0 alpha:1];
    [_pStartButton addTarget:self action:@selector(clickStartButton:) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:_pStartButton];
    
    _pText = [[UITextField alloc] initWithFrame:CGRectMake(SCREENWIDTH_D40 * 20, SCREENHEIGHT_D40 * 23, SCREENWIDTH_D40 * 10, SCREENHEIGHT_D40 * 2)];
    _pText.textColor = [UIColor redColor];
    _pText.textAlignment = NSTextAlignmentCenter;
    [_pText setText:@"正常播放:"];
    [self.view addSubview:_pText];
    
    
    _pPauseSwitch = [[UISwitch alloc] init];
    _pPauseSwitch.frame = CGRectMake(SCREENWIDTH_D40 * 30, SCREENHEIGHT_D40 * 23, SCREENWIDTH_D40 * 10, SCREENHEIGHT_D40 * 2);
    _pPauseSwitch.on = NO;
    [_pPauseSwitch setOnTintColor:[UIColor orangeColor]];
    [_pPauseSwitch setThumbTintColor:[UIColor blueColor]];
    [_pPauseSwitch setTintColor:[UIColor greenColor]];
    [_pPauseSwitch addTarget:self action:@selector(clickPauseSwitch:) forControlEvents:UIControlEventValueChanged];
    [self.view addSubview:_pPauseSwitch];
    
    _pStopButton = [UIButton buttonWithType:UIButtonTypeSystem];
    _pStopButton.frame = CGRectMake(SCREENWIDTH_D40 * 30, SCREENHEIGHT_D40 * 26, SCREENWIDTH_D40 * 10, SCREENHEIGHT_D40 * 2);
    [_pStopButton setTitle:@"停止播放" forState:UIControlStateNormal];
    [_pStopButton setTitleColor:[UIColor colorWithRed:1 green:1 blue:1 alpha:1] forState:(UIControlState)UIControlStateNormal];
    _pStopButton.contentMode = UIViewContentModeCenter;
    _pStopButton.backgroundColor = [UIColor colorWithRed:0 green:1 blue:0 alpha:1];
    [_pStopButton addTarget:self action:@selector(clickStopButton:) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:_pStopButton];
    
    _pCurPos = [[UILabel alloc] initWithFrame:CGRectMake(SCREENWIDTH_D40 * 30, SCREENHEIGHT_D40 * 29, SCREENWIDTH_D40 * 10, SCREENHEIGHT_D40 * 2)];
    _pCurPos.textAlignment = NSTextAlignmentCenter;
    [self.view addSubview:_pCurPos];
    
    _pCurDuration = [[UILabel alloc] initWithFrame:CGRectMake(SCREENWIDTH_D40 * 30, SCREENHEIGHT_D40 * 32, SCREENWIDTH_D40 * 10, SCREENHEIGHT_D40 * 2)];
    _pCurDuration.textAlignment = NSTextAlignmentCenter;
    [self.view addSubview:_pCurDuration];
    
    
    _player = [[PPlayerMidlle alloc] initPlayer:[path UTF8String]];
    [_player setView:(__bridge void *)(playerView)];
    [_player prepareAsync];
    
    
    _timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0));
    dispatch_source_set_timer(_timer, DISPATCH_TIME_NOW, 0.5 * NSEC_PER_SEC, 0);
    dispatch_source_set_event_handler(_timer, ^{
        dispatch_async(dispatch_get_main_queue(), ^{
            float curPos = [_player getCurPos];
            _pCurPos.text = [NSString stringWithFormat:@"%f", curPos];
            float duration = [_player getDuration];
            _pCurDuration.text = [NSString stringWithFormat:@"%f", duration];

        });
    });
    dispatch_resume(_timer);
}

- (void)dealloc{
    NSLog(@"VideoViewController dealloc");
    _timer = nil;
}

- (void)clickStartButton:(id)sender {
    if(self.pStartButton == nil) {
        return;
    }
    [_player start];
}

- (void)clickPauseSwitch:(UISwitch *)sw {
    if(self.pPauseSwitch == nil) {
        return;
    }
    if (sw.on == YES) {
        [_pText setText:@"暂停播放:"];
        [_player pause:true];
    } else {
        [_pText setText:@"正常播放:"];
        [_player pause:false];
    }
}

- (void)clickStopButton:(id)sender {
    if(self.pStopButton == nil) {
        return;
    }
    
}
/**
 *  @fun:getFileFromMainbundleAbsolutePath 获取资源文件绝对路径，以mainbundle为基础路径进行路径拼接，
 *  @fileCompent: 资源文件存放目录的相对路径
 */
- (NSString*) getFileFromMainbundleAbsolutePath:(NSString*) fileCompent
{
    return [NSString stringWithFormat:@"%@/%@",[[NSBundle mainBundle] resourcePath], fileCompent];
}

@end

