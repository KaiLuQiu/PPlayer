//
//  PacketQueueFunc.h
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#ifndef PacketQueueFunc_H
#define PacketQueueFunc_H
#include "MediaCommon.h"
NS_MEDIA_BEGIN

class PacketQueueFunc
{
public:
    PacketQueueFunc(AVPacket *flush_pkt);
    ~PacketQueueFunc();
    
    void packet_queue_init(PacketQueue *q);            //队列初始化的过程
    
    void packet_queue_start(PacketQueue *q);           //启用
    
    int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block, int *serial);

    int packet_queue_put_private(PacketQueue *q, AVPacket *pkt);
    int packet_queue_put(PacketQueue *q, AVPacket *pkt);
    int packet_queue_put_nullpacket(PacketQueue *q, int stream_index);
 
    void packet_queue_flush(PacketQueue *q);
    void packet_queue_destroy(PacketQueue *q);
    void packet_queue_abort(PacketQueue *q);
private:
    AVPacket *flush_pkt;
};


NS_MEDIA_END
#endif // PacketQueueFunc.h
