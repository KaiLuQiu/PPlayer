//
//  PPlayer.h
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#ifndef PPLAYER_H
#define PPLAYER_H

#include <list>

typedef struct P_AVPacket_T {
    AVPacket pkt;
    int serial;
} P_AVPacket;

typedef struct PacketQueue_T {
    std::list<P_AVPacket *> AvPacketList;
    int nb_packets;
    int size;
    int64_t duration;
    int abort_request;
    int serial;
    SDL_mutex *mutex;
    SDL_cond *cond;
} PacketQueue;



#endif // PPLAYER_H
