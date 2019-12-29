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

@interface PPlayerController ()<OnPreparedListener, OnCompletionListener, OnSeekCompletionListener, OnErrorListener, OnInfoListener> {
    PPlayerMidlle               *_player;
    OpenGLView                  *playerView;
    dispatch_source_t           _timer;
    NSMutableArray              *_videoUrlArray;
}

@end

@implementation PPlayerController

- (void)viewWillDisappear:(BOOL)animated{
    [super viewWillDisappear:animated];
    dispatch_source_cancel(_timer);
}

- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = [UIColor whiteColor];
    [self InitGLView];
    [self InitText];
    [self InitLabel];
    [self InitButton];
    [self InitSlider];
    
    NSString* path = [self getFileFromMainbundleAbsolutePath:@"video/hiphop.mp4"];
    _player = [[PPlayerMidlle alloc] initPlayer:[path UTF8String]];
    [_player setView:(__bridge void *)(playerView)];
    
    // 设置监听器
    [self setPlayerStateListener];
    
    [self PrepareAsync];
    
    [self GetPlayerTimeInfo];
}

-(void)passViewController:(NSMutableArray *)urlArray
{
    _videoUrlArray = urlArray;
}

#pragma mark 初始化GLView
- (void) InitGLView {
    playerView = [[OpenGLView alloc] initWithFrame:CGRectMake(0, 0, self.view.frame.size.width, self.view.frame.size.height)];
    playerView.backgroundColor = [UIColor blackColor];
    [self.view addSubview:playerView];
}

#pragma mark 初始化label控件
- (void) InitLabel {
    _pCurPos = [[UILabel alloc] initWithFrame:CGRectMake(SCREENWIDTH_D40 * 30, SCREENHEIGHT_D40 * 29, SCREENWIDTH_D40 * 10, SCREENHEIGHT_D40 * 2)];
    _pCurPos.textAlignment = NSTextAlignmentCenter;
    [self.view addSubview:_pCurPos];
    
    _pCurDuration = [[UILabel alloc] initWithFrame:CGRectMake(SCREENWIDTH_D40 * 30, SCREENHEIGHT_D40 * 32, SCREENWIDTH_D40 * 10, SCREENHEIGHT_D40 * 2)];
    _pCurDuration.textAlignment = NSTextAlignmentCenter;
    [self.view addSubview:_pCurDuration];
    
    _pVolumeText = [[UILabel alloc] initWithFrame:CGRectMake(SCREENWIDTH_D40 * 10, SCREENHEIGHT_D40 * 38, SCREENWIDTH_D40 * 20, SCREENHEIGHT_D40 * 1)];
    _pVolumeText.textColor = [UIColor orangeColor];
    _pVolumeText.textAlignment = NSTextAlignmentCenter;
    [self.view addSubview:_pVolumeText];
    
    _pSeekText = [[UILabel alloc] initWithFrame:CGRectMake(SCREENWIDTH_D40 * 10, SCREENHEIGHT_D40 * 40, SCREENWIDTH_D40 * 20, SCREENHEIGHT_D40 * 1)];
    _pSeekText.textColor = [UIColor orangeColor];
    _pSeekText.textAlignment = NSTextAlignmentCenter;
    [self.view addSubview:_pSeekText];
}

#pragma mark 初始化文本控件
- (void) InitText {
    _pText = [[UITextField alloc] initWithFrame:CGRectMake(SCREENWIDTH_D40 * 20, SCREENHEIGHT_D40 * 23, SCREENWIDTH_D40 * 10, SCREENHEIGHT_D40 * 2)];
    _pText.textColor = [UIColor redColor];
    _pText.textAlignment = NSTextAlignmentCenter;
    [_pText setText:@"正常播放:"];
    [self.view addSubview:_pText];
}

#pragma mark 初始化滑杆控件
- (void) InitSlider {
    _pVolumeSilder = [[UISlider alloc] initWithFrame:CGRectMake(SCREENWIDTH_D40 * 10, SCREENHEIGHT_D40 * 35, SCREENWIDTH_D40 * 20, SCREENHEIGHT_D40 * 2)];
    _pVolumeSilder.continuous = YES;
    _pVolumeSilder.minimumValue = 0;
    _pVolumeSilder.maximumValue = 100;
    [_pVolumeSilder addTarget:self action:@selector(VolumeSlider:) forControlEvents:UIControlEventValueChanged];
    [self.view addSubview:_pVolumeSilder];
    
    _pSeekSilder = [[UISlider alloc] initWithFrame:CGRectMake(SCREENWIDTH_D40 * 10, SCREENHEIGHT_D40 * 37, SCREENWIDTH_D40 * 20, SCREENHEIGHT_D40 * 2)];
    _pSeekSilder.continuous = NO;
    _pSeekSilder.minimumValue = 0;
    _pSeekSilder.maximumValue = 100;
    [_pSeekSilder addTarget:self action:@selector(SeekSlider:) forControlEvents:UIControlEventValueChanged];
    [self.view addSubview:_pSeekSilder];
}

#pragma mark 初始化按钮控件
- (void) InitButton {
    _pStartButton = [UIButton buttonWithType:UIButtonTypeSystem];
    _pStartButton.frame = CGRectMake(SCREENWIDTH_D40 * 30, SCREENHEIGHT_D40 * 20, SCREENWIDTH_D40 * 10, SCREENHEIGHT_D40 * 2);
    [_pStartButton setTitle:@"开始播放" forState:UIControlStateNormal];
    [_pStartButton setTitleColor:[UIColor colorWithRed:1 green:1 blue:1 alpha:1] forState:(UIControlState)UIControlStateNormal];
    _pStartButton.contentMode = UIViewContentModeCenter;
    _pStartButton.backgroundColor = [UIColor colorWithRed:0 green:1 blue:0 alpha:1];
    [_pStartButton addTarget:self action:@selector(clickStartButton:) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:_pStartButton];

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
    
    _pSeekButton = [UIButton buttonWithType:UIButtonTypeSystem];
    _pSeekButton.frame = CGRectMake(SCREENWIDTH_D40 * 25, SCREENHEIGHT_D40 * 20, SCREENWIDTH_D40 * 10, SCREENHEIGHT_D40 * 2);
    [_pSeekButton setTitle:@"seek" forState:UIControlStateNormal];
    [_pSeekButton setTitleColor:[UIColor colorWithRed:1 green:1 blue:1 alpha:1] forState:(UIControlState)UIControlStateNormal];
    _pSeekButton.contentMode = UIViewContentModeCenter;
    _pSeekButton.backgroundColor = [UIColor colorWithRed:0 green:1 blue:0 alpha:1];
    [_pSeekButton addTarget:self action:@selector(clickSeekButton:) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:_pSeekButton];
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

- (void)clickSeekButton:(id)sender {
    if(self.pSeekButton == nil) {
        return;
    }
    [_player seek:0.5];
}

- (void)VolumeSlider:(id)sender {
    UISlider *slider = (UISlider *)sender;
    _pVolumeText.text = [NSString stringWithFormat:@"%.0f", slider.value];
    float value = slider.value;
    [_player setVolume:value];
}

- (void)SeekSlider:(id)sender {
    UISlider *slider = (UISlider *)sender;
    _pSeekText.text = [NSString stringWithFormat:@"%.0f", slider.value];
    float value = slider.value / 100.f;
    [_player seek:value];
}

- (void)GetPlayerTimeInfo {
    _timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0));
    dispatch_source_set_timer(_timer, DISPATCH_TIME_NOW, 0.5 * NSEC_PER_SEC, 0);
    dispatch_source_set_event_handler(_timer, ^{
        dispatch_async(dispatch_get_main_queue(), ^{
            int64_t curPos = [_player getCurPos];
            _pCurPos.text = [self timeToStr:curPos];
            int64_t duration = [_player getDuration];
            _pCurDuration.text = [self timeToStr:duration];
            _pSeekSilder.value = ((float) curPos / duration) * 100;
        });
    });
    dispatch_resume(_timer);
}

#pragma mark 时间转换工具
- (NSString *)timeToStr: (long)totalTime {
    NSString *totalStr = nil;
    NSInteger time = (NSInteger)totalTime;
    if (time < 60) {
        // 秒
        totalStr = [NSString stringWithFormat:@"00:00:%02ld", (long)time];
    } else if (time >= 60 && time < 60 * 60) {
        // 分钟
        totalStr = [NSString stringWithFormat:@"00:%02ld:%02ld", (long)time / 60, (long)time % 60];
    } else if (time >= 60 * 60) {
        // 小时
        totalStr = [NSString stringWithFormat:@"%02ld:%02ld:%02ld", (long)time / (60 * 60), ((long)time % (60 * 60)) / 60, (long)time % 60];
    }
    return totalStr;
}

/**
 *  @fun:getFileFromMainbundleAbsolutePath 获取资源文件绝对路径，以mainbundle为基础路径进行路径拼接，
 *  @fileCompent: 资源文件存放目录的相对路径
 */
- (NSString*) getFileFromMainbundleAbsolutePath:(NSString*) fileCompent
{
    return [NSString stringWithFormat:@"%@/%@",[[NSBundle mainBundle] resourcePath], fileCompent];
}

#pragma mark 设置监听器
-(void) setPlayerStateListener {
    [_player setOnPreparedListener:self];
    [_player setOnInfoListener:self];
    [_player setOnErrorListener:self];
    [_player setOnCompletionListener:self];
    [_player setOnSeekCompletionListener:self];
}

#pragma mark PrepareAsync
-(void)PrepareAsync{
    [_player prepareAsync];
}

#pragma mark PPlayer PROTOCOL
#pragma mark - OnPreparedListener
-(void) onPrepared {
    printf("pplayer: Prepared !!!\n");
}

#pragma mark - OnCompletionListener
-(void) onCompletion {
    
}

#pragma mark - OnSeekCompletionListener
-(void) OnSeekCompletion {
    
}

#pragma mark - OnErrorListener
-(void) OnError {
    
}

#pragma mark - OnInfoListener
-(void) OnInfo {
    
}
@end

