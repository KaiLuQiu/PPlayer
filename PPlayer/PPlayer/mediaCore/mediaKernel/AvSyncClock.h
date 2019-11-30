//
//  AyncClock.h
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#ifndef AyncClock_H
#define AyncClock_H
#include "MediaDefineInfo.h"

NS_MEDIA_BEGIN

#define AV_SYNC_THRESHOLD_MIN 0.04
#define AV_SYNC_THRESHOLD_MAX 0.1
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
#define AV_NOSYNC_THRESHOLD 10.0


class AvSyncClock
{
public:
    /*
     * 时钟初始化
     */
    static void init_clock(Clock *c, int *queue_serial);

    /*
     * 获取当前显示的pts(这个是一个估算值)
     */
    static double get_clock(Clock *c);

    /*
     * 更新clock
     */
    static void set_clock_at(Clock *c, double pts, int serial, double time);

    /*
     * 获取系统时间，更新clock
     */
    static void set_clock(Clock *c, double pts, int serial);
    
    /*
     * 设置播放速度，更新clock
     */
    static void set_clock_speed(Clock *c, double speed);
    
//    /*
//     * 用于同步外部时钟的
//     */
//    static void sync_clock_to_slave(Clock *c, Clock *slave);

//    /*
//     * 设置播放速度，更新clock
//     */
//    static int get_master_sync_type();

    /*
     * 获取当前的主时钟（目前只支持视频同步音频）
     */
    static double get_master_clock(PlayerContext *player);

private:
};


NS_MEDIA_END

#endif // AyncClock.h
