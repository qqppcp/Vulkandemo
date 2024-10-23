#pragma once

#include <functional>
#include <memory>
#include <unordered_map>

struct EventContext
{
	union 
	{
		long long int i64[2];
		unsigned long long int u64[2];
		double f64[2];

		int i32[4];
		unsigned int u32[4];
		float f32[4];

		short i16[8];
		unsigned short u16[8];

		signed char i8[16];
		unsigned char u8[16];

		char c[16];
	} data;
};

typedef  bool (*PFN_ON_EVENT)(unsigned short code, void* sender, void* listener_inst, EventContext data);

enum EVENTCODE
{
	APPLICATION_QUIT = 0x01,
	KEY_PRESSED = 0x02,
	KEY_RELEASED = 0x03,
	BUTTON_PRESSED = 0x04,
	BUTTON_RELEASED = 0x05,
	MOUSE_MOVED = 0x06,
	MOUSE_WHEEL = 0x07,
	RESIZED = 0x08,
	SET_RENDER_MODE = 0x0A,
	DEBUG0 = 0x10,
	DEBUG1 = 0x11,
	DEBUG2 = 0x12,
	DEBUG3 = 0x13,
	DEBUG4 = 0x14,
	MAX_EVENT_CODE = 0xFF
};

class EventManager 
{
public:
	static void Init();
	static void Shutdown();
	static EventManager& GetInstance();
	
	bool Register(unsigned short code, void* listener, PFN_ON_EVENT on_event);
	bool Unregister(unsigned short code, void* listener, PFN_ON_EVENT on_event);
	bool Fire(unsigned short code, void* sender, EventContext context);

private:
	EventManager() {};
	struct Event
	{
		void* listener;
		PFN_ON_EVENT callback;
	};
	struct EventCodeEntry
	{
		std::vector<Event> events;
	};
	std::unordered_map<unsigned short, EventCodeEntry> registered;
	static std::unique_ptr<EventManager> instance_;
};