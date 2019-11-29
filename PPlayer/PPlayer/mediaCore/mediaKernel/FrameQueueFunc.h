//
//  FrameQueue.h
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#ifndef FrameQueue_H
#define FrameQueue_H
#include "MediaDefineInfo.h"
NS_MEDIA_BEGIN

class FrameQueueFunc
{
public:
    /*
     * 取消帧引用的所有缓冲区并重置帧字段，释放给定字幕结构中的所有已分配数据。
     */
    static void frame_queue_unref_item(Frame *vp);
    
    /*
     * Framequeue的初始化
     */
    static int frame_queue_init(FrameQueue *f, PacketQueue *pktq, int max_size, int keep_last);
    
    /*
     * 释放Frame，释放互斥锁和互斥量
     */
    static void frame_queue_destory(FrameQueue *f);
    
    /*
     * 获取待显示的第一个帧
     */
    static Frame *frame_queue_peek(FrameQueue *f);
    
    /*
     * 获取待显示的第二个帧
     */
    static Frame *frame_queue_peek_next(FrameQueue *f);
    
    /*
     * 获取当前播放器显示的帧
     */
    static Frame *frame_queue_peek_last(FrameQueue *f);
    
    /*
     * 获取queue中一块Frame大小的可写内存
     */
    static Frame *frame_queue_peek_writable(FrameQueue *f);
    
    /*
     * 这方法和frame_queue_peek的作用一样， 都是获取待显示的第一帧
     */
    static Frame *frame_queue_peek_readable(FrameQueue *f);
    
    /*
     * 推入一帧数据， 其实数据已经在调用这个方法前填充进去了， 这个方法的作用是将队列的写索引(也就是队尾)向后移， 还有将这个队列中的Frame的数量加一。
     */
    static void frame_queue_push(FrameQueue *f);
    
    /*
     * 将读索引(队头)后移一位， 还有将这个队列中的Frame的数量减一
     */
    static void frame_queue_next(FrameQueue *f);
    
    /*
     * 返回队列中待显示帧的数目
     */
    static int frame_queue_nb_remaining(FrameQueue *f);
    
    /*
     * 返回正在显示的帧的position
     */
    static int64_t frame_queue_last_pos(FrameQueue *f);
private:
};

NS_MEDIA_END


#endif // FrameQueue.h
