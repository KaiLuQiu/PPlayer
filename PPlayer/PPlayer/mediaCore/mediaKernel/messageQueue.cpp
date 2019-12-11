//
//  messageQueue.cpp
//  PPlayer
//
//  Created by 邱开禄 on 2019/12/06.
//  Copyright © 2019 邱开禄. All rights reserved.
//
#include "messageQueue.hpp"

message::message()
{
    messageQueue = new std::list<msgInfo>();
    if (messageQueue == NULL) {
        printf("message Queue is NULL!!!\n");
    }
}

message::~message()
{
    if (messageQueue != NULL) {
        messageQueue->clear();
        delete messageQueue;
    }
}

int message::message_queue(msgInfo msg)
{
    mutex.lock();
    messageQueue->push_back(msg);
    mutex.unlock();
    return true;
}

void message::message_dequeue(msgInfo &msg)
{
    mutex.lock();
    if (!messageQueue->empty()) {
        msg = messageQueue->front();
        messageQueue->pop_front();
    } else {
        msg.cmd = MESSAGE_CMD_NONE;
        msg.data = -1;
    }
    mutex.unlock();
}

int message::message_cmd_size()
{
    return messageQueue->size();
}

int message::message_is_empty()
{
    return messageQueue->empty();
}

