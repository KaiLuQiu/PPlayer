//
//  EventHandler.cpp
//  PPlayer
//
//  Created by 邱开禄 on 2019/12/13.
//  Copyright © 2019 邱开禄. All rights reserved.
//


#include <chrono>
#include <algorithm>
#include <iostream>
#include "EventHandler.h"
#include "Message.h"
#include "PPlayer.h"

NS_MEDIA_BEGIN
#define LOGENTER (std::cout << "This is FUNCTION " << __func__<<  std::endl)
EventHandler::EventHandler():stop(false),stopWhenEmpty(false)
{
    // lamda表达式，创建一个looper线程
    mediaPlayer = NULL;
	looper = std::thread( [this]() -> void{
			for(;;)
			{
				Message msg;
				{
					std::unique_lock<std::mutex> lock(this->queue_mutex);
                    if(this->msg_Q.empty())
                    {
                        this->condition.wait(lock, [this] { return this->stop || this->stopWhenEmpty || !this->msg_Q.empty();});
                    }
                    else
                    {
                        this->condition.wait_until(lock, this->msg_Q.back().when, [this]{ return this->stop || this->stopWhenEmpty || !this->msg_Q.empty(); });
                    }

					if(this->stopWhenEmpty && this->msg_Q.empty())
						return;
                    
					if(stop)
                    {
						msg_Q.clear();
						return;
					}

					msg = std::move(msg_Q.back());
					msg_Q.pop_back();
				}
				this->dispatchMessage(msg);
			}
		});
}

EventHandler::~EventHandler()
{
	{
		std::unique_lock<std::mutex> lock(queue_mutex);
		stop = true;
	}
	condition.notify_all();
	looper.join();
	msg_Q.clear();

}

void EventHandler::setMediaPlayer(PPlayer *player)
{
    mediaPlayer = player;
}

void EventHandler::handleMessage(Message& msg)
{
    if(NULL != mediaPlayer) {
        mediaPlayer->mEventHandler(msg);
    }
    printf("in Hander %d\n!!!", msg.m_what);
}

bool EventHandler::sendMessageAtTime(Message& msg, long uptimeMillis)
{
	if(uptimeMillis < 0 )
		return false;

	msg.setWhen(uptimeMillis);

	std::unique_lock<std::mutex> lock(queue_mutex);
	auto i = std::find(msg_Q.begin(),msg_Q.end(),msg);
	msg_Q.erase(i);

	msg_Q.push_back(msg);
	std::sort(msg_Q.begin(), msg_Q.end(),std::greater<Message>());
	condition.notify_one();
	return true;
}

bool EventHandler::sendMessage(Message& msg)
{
	std::unique_lock<std::mutex> lock(queue_mutex);
	auto i = find(msg_Q.begin(),msg_Q.end(),msg);
	if(i != msg_Q.end())
		msg_Q.erase(i);

	msg_Q.push_back(msg);
	std::sort(msg_Q.begin(), msg_Q.end(),std::greater<Message>());
	condition.notify_one();
	return true;
}

bool EventHandler::sendEmptyMessage(int what)
{
	return sendEmptyMessage(what ,0);
}

bool EventHandler::sendEmptyMessage(int what,long uptimeMillis)
{

	if(what < 0 || uptimeMillis < 0)
		return false;

	Message msg(what);
	msg.setWhen(uptimeMillis);

	std::unique_lock<std::mutex> lock(queue_mutex);

	std::vector<Message>::iterator i = find(msg_Q.begin(),msg_Q.end(),msg);
	if (i != msg_Q.end()){
		msg_Q.erase(i);
	}

	msg_Q.push_back(msg);
	// 跟进时间进行降序排列
	std::sort(msg_Q.begin(), msg_Q.end(),std::greater<Message>());

	condition.notify_one();
	return true;
}

bool EventHandler::post(Message::Function f)
{
	return postAtTime(f,0);
}

bool EventHandler::postAtTime(Message::Function f, long uptimeMillis)
{

	if(f == nullptr || uptimeMillis < 0){
		return false;
	}

	std::unique_lock<std::mutex> lock(queue_mutex);
	Message msg;
	msg.setWhen(uptimeMillis);
	msg.setFunction(f);
	msg_Q.push_back(msg);
	std::sort(msg_Q.begin(), msg_Q.end(),std::greater<Message>());

	return true;
}

void EventHandler::removeMessages(int what)
{
	if(what < 0)
		return;

	std::unique_lock<std::mutex> lock(queue_mutex);

	auto i = find(msg_Q.begin(),msg_Q.end(),what);
	if (i != msg_Q.end()){
		msg_Q.erase(i);
	}

	condition.notify_one();
}

void EventHandler::removeCallbackAndMessages()
{
	std::unique_lock<std::mutex> lock(queue_mutex);
	msg_Q.clear();
}

void EventHandler::stopSafty(bool stopSafty)
{
	std::unique_lock<std::mutex> lock(queue_mutex);
	if(stopSafty){
		stopWhenEmpty = true;
	}else{
		stop = true;
	}
}


bool EventHandler::isQuiting()
{
	std::unique_lock<std::mutex> lock(queue_mutex);
	if(stop || stopWhenEmpty)
		return true;
	return false;
}

void EventHandler::dispatchMessage(Message& msg)
{
	if(msg.task != nullptr){
		msg.task();
	}else{
		if(msg.m_what < 0)
			return;
		handleMessage(msg);
	}
}

NS_MEDIA_END
