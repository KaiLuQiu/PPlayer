//
//  AyncClock.cpp
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#include "AvSyncClock.h"

NS_MEDIA_BEGIN
void AvSyncClock::init_clock(Clock *c, int *queue_serial)
{
    c->speed = 1.0;
    c->paused = 0;
    c->queue_serial = queue_serial;
    set_clock(c, NAN, -1);
}

double AvSyncClock::get_clock(Clock *c)
{
    if (*c->queue_serial != c->serial)
        return NAN;
    if (c->paused) {
        return c->pts;
    } else {
        // 获取当前系统的时间
        double time = av_gettime_relative() / 1000000.0;
        // 获取当前显示的时间
        return c->pts_drift + time - (time - c->last_updated) * (1.0 - c->speed);
    }
}
 
void AvSyncClock::set_clock_at(Clock *c, double pts, int serial, double time)
{
    c->pts = pts;
    c->last_updated = time;
    c->pts_drift = c->pts - time;
    c->serial = serial;
}

void AvSyncClock::set_clock(Clock *c, double pts, int serial)
{
    double time = av_gettime_relative() / 1000000.0;
    set_clock_at(c, pts, serial, time);
}

void AvSyncClock::set_clock_speed(Clock *c, double speed)
{
    set_clock(c, get_clock(c), c->serial);
    c->speed = speed;
}


// 因为是视频同步音频，因此获取的要是音频的时钟
double AvSyncClock::get_master_clock(PlayerContext *player)
{
    double val = get_clock(&player->AudioClock);
    return val;
}



NS_MEDIA_END
