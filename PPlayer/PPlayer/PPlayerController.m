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

}

@end

@implementation PPlayerController

- (void)viewWillDisappear:(BOOL)animated{
    [super viewWillDisappear:animated];
}

- (void)viewDidLoad {
    [super viewDidLoad];
    NSString* path = [self getFileFromMainbundleAbsolutePath:@"video/hiphop.mp4"];
    self.view.backgroundColor = [UIColor whiteColor];
    OpenGLView *playerView = [[OpenGLView alloc] initWithFrame:CGRectMake(0, 0, self.view.frame.size.width, self.view.frame.size.height)];
    playerView.backgroundColor = [UIColor blackColor];
    [self.view addSubview:playerView];
    
    PPlayerMidlle *player = [[PPlayerMidlle alloc] initPlayer:[path UTF8String]];
    [player setView:(__bridge void *)(playerView)];
    [player prepareAsync];
    [player start];

}

- (void)dealloc{
    NSLog(@"VideoViewController dealloc");

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

