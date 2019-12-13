//
//  PacketQueue.cpp
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#include "PacketQueueFunc.h"
NS_MEDIA_BEGIN

PacketQueueFunc::PacketQueueFunc(AVPacket *flush_pkt)
{
    this->flush_pkt = flush_pkt;
}

PacketQueueFunc::~PacketQueueFunc()
{
    
}

void PacketQueueFunc::packet_queue_init(PacketQueue *q)
{

    if(!q->mutex) {
        q->mutex = SDL_CreateMutex();
        if (!q->mutex) {
            printf("SDL_CreateMutex() : %s\n",SDL_GetError());
            return ;
        }
    }
    if(!q->cond) {
        q->cond = SDL_CreateCond();
        if (!q->cond) {
            printf("SDL_CreateCond() : %s\n",SDL_GetError());
            return ;
        }
    }
    q->abort_request = 1;
    return ;
}

void PacketQueueFunc::packet_queue_start(PacketQueue *q)
{
    SDL_LockMutex(q->mutex);
    q->abort_request = 0;
    packet_queue_put_private(q, flush_pkt);
    SDL_UnlockMutex(q->mutex);
}

int PacketQueueFunc::packet_queue_get(PacketQueue *q, AVPacket *pkt, int block, int *serial)
{
    int ret;
    P_AVPacket  *p_pkt;
    SDL_LockMutex(q->mutex);
    
    if(q == NULL || pkt == NULL)        //添加容错处理
    {
        SDL_UnlockMutex(q->mutex);
        return -1;
    }

    for(;;)
    {
        if (q->abort_request) {
            ret = -1;
            break;
        }
        
        p_pkt = q->AvPacketList.front();
        q->AvPacketList.pop_front();
        if(p_pkt){
            q->nb_packets--;
            q->size -= p_pkt->pkt.size + sizeof(*p_pkt);
            q->duration -= p_pkt->pkt.duration;
            *pkt = p_pkt->pkt;
            if(serial)
                *serial = p_pkt->serial;
            av_free(p_pkt);
            ret = 1;
            break;
        } else if(!block) {
            ret = 0;
            break;
        } else {
            SDL_CondWait(q->cond, q->mutex);
        }
    }

    SDL_UnlockMutex(q->mutex);
    return ret;
}

int PacketQueueFunc::packet_queue_put_private(PacketQueue *q, AVPacket *pkt)
{
    if (q->abort_request)//如果已中止，则放入失败
        return -1;
    
    q->AvPacketList;
    P_AVPacket *p_pkt = new P_AVPacket();       //创建一个新的P_AVPacket节点
    if(!p_pkt)
        return -1;
    p_pkt->pkt = *pkt;
    if(pkt == flush_pkt)                        //
        q->serial++;
    p_pkt->serial = q->serial;              //使用同一个序列号
    
    q->AvPacketList.push_back(p_pkt);       //把当前的这个P_AVPacket丢进队列中
    q->nb_packets++;                        //添加节点数据
    q->size += p_pkt->pkt.size + sizeof(*p_pkt);  // 计算包的size大小以及p_pkt结构大小
    q->duration += p_pkt->pkt.duration;
    
    SDL_CondSignal(q->cond);
    return 0;
}

int PacketQueueFunc::packet_queue_put(PacketQueue *q, AVPacket *pkt)
{
    int ret;
    SDL_LockMutex(q->mutex);
    ret = packet_queue_put_private(q, pkt);//主要实现在这里
    SDL_UnlockMutex(q->mutex);
    if (pkt != flush_pkt && ret < 0)
        av_packet_unref(pkt);//放入失败，释放AVPacket
    return ret;
    
}

int PacketQueueFunc::packet_queue_put_nullpacket(PacketQueue *q, int stream_index)
{
    AVPacket pkt1, *pkt = &pkt1;
    av_init_packet(pkt);
    pkt->data = NULL;
    pkt->size = 0;
    pkt->stream_index = stream_index;
    return packet_queue_put(q, pkt);
}

void PacketQueueFunc::packet_queue_flush(PacketQueue *q)
{
    SDL_LockMutex(q->mutex);

    std::list<P_AVPacket *>::iterator item = q->AvPacketList.begin();
    for(; item != q->AvPacketList.end(); )
    {
        std::list<P_AVPacket *>::iterator item_e = item++;
        av_packet_unref(&(*item_e)->pkt);           //先释放avpacket
        av_freep(&(*item_e));
        q->AvPacketList.erase(item_e);
    }
    q->AvPacketList.clear();
    q->nb_packets = 0;
    q->size = 0;
    q->duration = 0;
    SDL_UnlockMutex(q->mutex);
}

void PacketQueueFunc::packet_queue_destroy(PacketQueue *q)
{
    packet_queue_flush(q);
}

void PacketQueueFunc::packet_queue_abort(PacketQueue *q)
{
    SDL_LockMutex(q->mutex);
    q->abort_request = 1;
    SDL_UnlockMutex(q->mutex);
    SDL_CondSignal(q->cond);
}


NS_MEDIA_END
