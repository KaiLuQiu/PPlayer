//
//  FrameQueue.cpp
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#include "FrameQueueFunc.h"
NS_MEDIA_BEGIN

// 取消帧引用的所有缓冲区并重置帧字段，释放给定字幕结构中的所有已分配数据。
// frame_queue_unref_item释放的内存都是关联的内存，而非结构体自身内存。
// AVFrame内部有许多的AVBufferRef类型字段，而AVBufferRef只是AVBuffer的引用，AVBuffer通过引用计数自动管理内存（简易垃圾回收机制）。因此AVFrame在不需要的时候，需要通过av_frame_unref减少引用计数。
void FrameQueueFunc::frame_queue_unref_item(Frame *vp)
{
    av_frame_unref(vp->frame);      // 释放帧引用
    avsubtitle_free(&vp->sub);      // 释放sub的结构
}

// Framequeue的初始化
int FrameQueueFunc::frame_queue_init(FrameQueue *f, PacketQueue *pktq, int max_size, int keep_last)
{
    int i;

    if(!f->mutex) {
        f->mutex = SDL_CreateMutex();
        if (!f->mutex) {
            printf("SDL_CreateMutex() : %s\n",SDL_GetError());
            return -1;
        }
    }
    if(!f->cond) {
        f->cond = SDL_CreateCond();
        if (!f->cond) {
            printf("SDL_CreateCond() : %s\n",SDL_GetError());
            return -1;
        }
    }
    
    f->pktq = pktq;
    f->max_size = FFMIN(max_size, FRAME_QUEUE_SIZE);
    f->keep_last = !!keep_last;
    for (i = 0; i < f->max_size; i++)
        if (!(f->queue[i].frame = av_frame_alloc()))
            return AVERROR(ENOMEM);
    return 0;
}

// 释放Frame，释放互斥锁和互斥量
void FrameQueueFunc::frame_queue_destory(FrameQueue *f)
{
    int i;
    for (i = 0; i < f->max_size; i++) {
        Frame *vp = &f->queue[i];
        frame_queue_unref_item(vp);
        av_frame_free(&vp->frame);      // 释放AVFrame.
    }
    SDL_DestroyCond(f->cond);
    SDL_DestroyMutex(f->mutex);
}

// 获取待显示的第一个帧 ,rindex 表示当前的显示的帧，rindex_shown表示当前带显示的是哪一帧
Frame *FrameQueueFunc::frame_queue_peek(FrameQueue *f)
{
    return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

// 获取待显示的第二个帧
Frame *FrameQueueFunc::frame_queue_peek_next(FrameQueue *f)
{
    return &f->queue[(f->rindex + f->rindex_shown + 1) % f->max_size];
}

// 获取当前播放器显示的帧
Frame *FrameQueueFunc::frame_queue_peek_last(FrameQueue *f)
{
    return &f->queue[f->rindex];
}

// 获取queue中一块Frame大小的可写内存
// 加锁情况下，等待直到队列有空余空间可写（f->size < f->max_size）
// 如果有退出请求（f->pktq->abort_request），则返回NULL
// 返回windex位置的元素（windex指向当前应写位置）
Frame *FrameQueueFunc::frame_queue_peek_writable(FrameQueue *f)
{
    SDL_LockMutex(f->mutex);
    while (f->size >= f->max_size &&
           !f->pktq->abort_request) {
        SDL_CondWait(f->cond, f->mutex);
    }
    SDL_UnlockMutex(f->mutex);
    
    if (f->pktq->abort_request)
        return NULL;
    
    return &f->queue[f->windex];

}

// 这方法和frame_queue_peek的作用一样， 都是获取待显示的第一帧
// 加锁情况下，判断是否有可读节点（f->size - f->rindex_shown > 0)
// 如果有退出请求，则返回NULL
// 读取当前可读节点(f->rindex + f->rindex_shown) % f->max_size
// rindex_shown的意思是rindex指向的节点是否被读过
Frame *FrameQueueFunc::frame_queue_peek_readable(FrameQueue *f)
{
    SDL_LockMutex(f->mutex);
    while (f->size - f->rindex_shown <= 0 &&
           !f->pktq->abort_request) {
        SDL_CondWait(f->cond, f->mutex);
    }
    SDL_UnlockMutex(f->mutex);
    
    if (f->pktq->abort_request)
        return NULL;
    
    return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

// 推入一帧数据， 其实数据已经在调用这个方法前填充进去了， 这个方法的作用是将队列的写索引(也就是队尾)向后移， 还有将这个队列中的Frame的数量加一。
// windex加1，如果超过max_size，则回环为0
// 加锁情况下大小加1.
void FrameQueueFunc::frame_queue_push(FrameQueue *f)
{
    if (++f->windex == f->max_size)  // 这边只要保证windex之前都都是被写过的
        f->windex = 0;
    SDL_LockMutex(f->mutex);
    f->size++;
    SDL_CondSignal(f->cond);
    SDL_UnlockMutex(f->mutex);
}

// 将读索引(队头)后移一位， 还有将这个队列中的Frame的数量减一
void FrameQueueFunc::frame_queue_next(FrameQueue *f)
{
    if (f->keep_last && !f->rindex_shown) {
        f->rindex_shown = 1;
        return;
    }
    frame_queue_unref_item(&f->queue[f->rindex]);
    if (++f->rindex == f->max_size)
        f->rindex = 0;
    SDL_LockMutex(f->mutex);
    f->size--;
    SDL_CondSignal(f->cond);
    SDL_UnlockMutex(f->mutex);
}

// 返回队列中待显示帧的数目
int FrameQueueFunc::frame_queue_nb_remaining(FrameQueue *f)
{
    return f->size - f->rindex_shown;
}

// 返回正在显示的帧的position
int64_t FrameQueueFunc::frame_queue_last_pos(FrameQueue *f)
{
    Frame *fp = &f->queue[f->rindex];
    if (f->rindex_shown && fp->serial == f->pktq->serial)
        return fp->pos;
    else
        return -1;
}

NS_MEDIA_END
