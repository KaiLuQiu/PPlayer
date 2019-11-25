//
//  OpenGLView.h
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#import <GLKit/GLKit.h>
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
#import "render_frame.h"

@interface OpenGLView : GLKView

- (void)Render:(VideoFrame *)frame;

@end
