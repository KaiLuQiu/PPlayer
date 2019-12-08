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
    Queue = new std::list<MessageCmd>();
    if (Queue == NULL)
    {
    }
}

message::~message()
{
    if (Queue != NULL)
    {
        Queue->clear();
        delete Queue;
    }
}

int message::message_queue(MessageCmd cmd)
{
    mutex.lock();
    Queue->push_back(cmd);
    mutex.unlock();
    
    return true;
}

int message::message_dequeue(MessageCmd *cmd)
{
    mutex.lock();
    if (!Queue->empty())
    {
        *cmd = Queue->front();
        Queue->pop_front();
    }
    else
    {
        mutex.unlock();
        return false;
    }
    mutex.unlock();
    
    return true;
}

int message::message_cmd_size()
{
    return Queue->size();
}

int message::message_is_empty()
{
    return Queue->empty();
}

