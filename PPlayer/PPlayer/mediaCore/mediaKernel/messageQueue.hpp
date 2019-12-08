//
//  messageQueue.h
//  PPlayer
//
//  Created by 邱开禄 on 2019/12/06.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#ifndef MESSAGEQUEUE_H
#define MESSAGEQUEUE_H
#include <list>
#include <mutex>

enum {
    MESSAGE_CMD_NONE,
    MESSAGE_CMD_PAUSE,
    MESSAGE_CMD_RESUME,
    MESSAGE_CMD_SEEK,
    MESSAGE_CMD_STOP,
    MESSAGE_CMD_SEL_TRACK,
    MESSAGE_CMD_FLUSH,
    MESSAGE_CMD_DECODE,
    MESSAGE_CMD_DMX,
    MESSAGE_CMD_WAIT_RBUF,
    MESSAGE_CMD_REINIT,
    MESSAGE_CMD_RESYNC,
    MESSAGE_CMD_REINIT_AUD_DRV,
    MESSAGE_CMD_RENDER,
    MESSAGE_CMD_INPUT_TYPE_CHANGE,
    MESSAGE_CMD_FORCE_EOF,
    MESSAGE_CMD_CHANGE_SPEED,
};

typedef enum
{
    MESSAGE_CMD_QUEUE = 0,
    MESSAGE_NOTIFY_QUEUE,
    MESSAGE_QUEUE_TYPES
} MessageQueueType;

typedef struct
{
    MessageQueueType cmdType;
    int cmd;
} MessageCmd;

class message
{
public:
    message();
    virtual ~message();
    int message_queue(MessageCmd cmd);
    int message_dequeue(MessageCmd *cmd);
    int message_cmd_size();
    int message_is_empty();
private:
    std::mutex mutex;
    std::list<MessageCmd> *Queue;
};


#endif // MESSAGEQUEUE_H
