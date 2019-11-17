//
//  PacketQueue.h
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#ifndef PacketQueue_H
#define PacketQueue_H
#include "MediaDefineInfo.h"
NS_MEDIA_BEGIN

class PacketQueueFunc
{
public:
    
    static void packet_queue_init(PacketQueue *q);            //队列初始化的过程
    
    static void packet_queue_start(PacketQueue *q, AVPacket *flush_pkt);           //启用
    
    static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block, int *serial);

    static int packet_queue_put_private(PacketQueue *q, AVPacket *pkt, AVPacket *flush_pkt);
    static int packet_queue_put(PacketQueue *q, AVPacket *pkt, AVPacket *flush_pkt);
    static int packet_queue_put_nullpacket(PacketQueue *q, int stream_index, AVPacket *flush_pkt);
;
    
    static void packet_queue_flush(PacketQueue *q);
    static void packet_queue_destroy(PacketQueue *q);
    static void packet_queue_abort(PacketQueue *q);

    
};


NS_MEDIA_END
#endif // FrameQueue.h
