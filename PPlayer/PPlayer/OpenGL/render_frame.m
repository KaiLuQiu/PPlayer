//
//  render_frame.m
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//


#include "render_frame.h"
#include "OpenGLView.h"
void Render(void *view,VideoFrame *frame){
    @autoreleasepool {
        OpenGLView *glView = (__bridge OpenGLView *)view;
        [glView Render:frame];
    }
}
