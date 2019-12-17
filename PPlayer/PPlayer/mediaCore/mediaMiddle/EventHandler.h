//
//  EventHandler.h
//  PPlayer
//
//  Created by 邱开禄 on 2019/12/13.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#pragma once
#ifndef EventHandler_H
#define EventHandler_H
#include "MediaCommon.h"
#include <chrono>
#include <map>
#include <vector>
#include <thread>
#include "Message.h"
#include "PPlayer_C_Interface.h"

NS_MEDIA_BEGIN

class EventHandler{
public:
	EventHandler();
	virtual ~EventHandler();
    
    void setMediaPlayer(void *playerInstance, msg_loop callback);

    void sendOnStart();
    
    void sendOnPause();
    
    void sendOnPrepared();
    
    void sendOnCompletion();
    
    void sendOnSeekCompletion();
    
    void sendOnSeekFail();
    
    void sendOnError(int what, int arg1);
    
    void sendOnInfo(int what, int arg1);
    
	bool sendMessageAtTime(Message& msg, long uptimeMillis);
    
	bool sendMessage(Message& msg);
    
	bool sendEmptyMessage(int what);
    
	bool sendEmptyMessage(int what, long uptimeMillis);

	bool post(Message::Function f);
    
	bool postAtTime(Message::Function f, long uptimeMillis);

	void removeMessages(int what);
    
	void removeCallbackAndMessages();

	void stopSafty(bool stopSafty);

	bool isQuiting();

	virtual void handleMessage(Message& msg);

	void dispatchMessage(Message& msg);

    
	template<class T>
	class ValComp {
	public:
		bool operator()(const T& t1,const T& t2) const {
			return (t1 < t2);
		}

	};

private:
	std::vector<Message> msg_Q;
    std::mutex queue_mutex;
    void *mediaPlayer;
    msg_loop msgLoopCallBack;
	std::condition_variable condition;
	std::thread looper;
	bool stop;
	bool stopWhenEmpty;
};

NS_MEDIA_END
#endif
