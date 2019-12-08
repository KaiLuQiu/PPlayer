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
    messageQueue = new std::list<MessageCmd>();
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

int message::message_queue(MessageCmd cmd)
{
    mutex.lock();
    messageQueue->push_back(cmd);
    mutex.unlock();
    return true;
}

void message::message_dequeue(MessageCmd &cmd)
{
    mutex.lock();
    if (!messageQueue->empty()) {
        cmd = messageQueue->front();
        messageQueue->pop_front();
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

