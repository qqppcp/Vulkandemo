#include "event.h"

#include "log.h"
#include <string>
#include <format>

std::unique_ptr<EventManager> EventManager::instance_ = nullptr;
#define MAX_MESSAGE_CODES 16384

void EventManager::Init()
{
	if (!instance_)
	{
		instance_.reset(new EventManager);
	}
}

void EventManager::Shutdown()
{
	if (instance_)
	{
		instance_->registered.clear();
		instance_.reset();
	}
}

EventManager& EventManager::GetInstance()
{
	if (!instance_)
	{
		DEMO_LOG(Error, "EventManager can't get the handle before Init!");
	}
	return *instance_;
}

bool EventManager::Register(unsigned short code, void* listener, PFN_ON_EVENT on_event)
{
	if (registered.find(code) == registered.end())
	{
		registered[code] = {};
	}
	auto count = registered[code].events.size();
	for (size_t i = 0; i < count; i++)
	{
		if (registered[code].events[i].listener == listener && registered[code].events[i].callback == on_event)
		{
			DEMO_LOG(Warning, std::format("Event has already been registered with the code {} and the callback of {}", code, (std::uint64_t)on_event));
			return false;
		}
	}
	Event event{ .listener = listener, .callback = on_event };
	registered[code].events.push_back(event);
	return true;
}

bool EventManager::Unregister(unsigned short code, void* listener, PFN_ON_EVENT on_event)
{
	if (registered.find(code) == registered.end())
	{
		return false;
	}
	for (auto it = registered[code].events.begin(); it != registered[code].events.end(); ++it)
	{
		if (it->listener == listener && it->callback == on_event)
		{
			registered[code].events.erase(it);
			return true;
		}
	}
	return false;
}

bool EventManager::Fire(unsigned short code, void* sender, EventContext context)
{
	if (registered.find(code) == registered.end())
	{
		return false;
	}
	auto count = registered[code].events.size();
	for (size_t i = 0; i < count; i++)
	{
		auto e = registered[code].events[i];
		if (e.callback(code, sender, e.listener, context))
		{
			return true;
		}
	}
	return false;
}