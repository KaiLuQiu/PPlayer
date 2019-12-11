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

typedef enum {
    MESSAGE_CMD_NONE,
    MESSAGE_CMD_START,
    MESSAGE_CMD_PAUSE,
    MESSAGE_CMD_RESUME,
    MESSAGE_CMD_SEEK,
    MESSAGE_CMD_STOP,
    MESSAGE_CMD_FLUSH,
    MESSAGE_CMD_REINIT,
    MESSAGE_CMD_RESYNC,
    MESSAGE_CMD_REINIT_AUD_DRV,
    MESSAGE_CMD_FORCE_EOF,
    MESSAGE_CMD_CHANGE_SPEED,
}MessageCmd;

typedef struct msgInfo_T{
    msgInfo_T() {
        cmd = MESSAGE_CMD_NONE;
        data = -1;
    }
    ~msgInfo_T() {
        cmd = MESSAGE_CMD_NONE;
        data = -1;
    }
    MessageCmd cmd;
    float data;
}msgInfo;

class message
{
public:
    message();
    virtual ~message();
    int message_queue(msgInfo cmd);
    void message_dequeue(msgInfo &cmd);
    int message_cmd_size();
    int message_is_empty();
private:
    std::mutex mutex;
    std::list<msgInfo> *messageQueue;
};


#endif // MESSAGEQUEUE_H
