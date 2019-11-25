//
//  render_frame.h
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//


#ifndef display_frame_h
#define display_frame_h

#include <stdio.h>
#include <libavutil/frame.h>
#include <MacTypes.h>

typedef struct VideoFrame VideoFrame;

struct VideoFrame {
    int width;
    int height;
    unsigned char *pixels[AV_NUM_DATA_POINTERS];
    int pitches[AV_NUM_DATA_POINTERS];
    int planar;
    enum AVPixelFormat format;
};
void Render(void *view,VideoFrame *frame);
#endif /* render_frame.h */
