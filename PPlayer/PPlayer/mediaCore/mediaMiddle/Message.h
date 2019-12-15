//
//  Message.h
//  PPlayer
//
//  Created by 邱开禄 on 2019/12/13.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#pragma once
#ifndef MESSAGE_H
#define MESSAGE_H
#include "MediaCommon.h"

#include <chrono>
#include <functional>
NS_MEDIA_BEGIN

class Message{
public:
    int m_what;
    int m_arg1;
    int m_arg2;
    typedef std::function<void()> Function;
    Function task;

    std::chrono::system_clock::time_point when;

public:
	Message();
	Message(int what);
	Message(int what, int arg1);
	Message(int what, int arg1, int arg2);
	Message(int what, int arg1, int arg2,long uptimeMillis);
    
	virtual ~Message();

	Message& operator=(const Message& msg);

	void setWhen(long uptimeMillis);

	void setFunction(Function f);

	bool operator > (const Message& msg) const {
		return (this->when > msg.when);
	}

	bool operator < (const Message& msg) const {
		return (this->when < msg.when);
	}

	bool operator==(const Message& msg) const {
		return (this->m_what == msg.m_what) && (this->task != nullptr) && (msg.task != nullptr);
	}

	bool operator==(int what) const {
		return (this->m_what == what);
	}

private:

};

NS_MEDIA_END
#endif
