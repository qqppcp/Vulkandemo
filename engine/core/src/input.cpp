#include "input.h"
#include "log.h"
#include "event.h"
#include <format>
namespace
{
	struct State
	{
		bool keys[400];
		short x;
		short y;
		bool buttons[3];
	};
	struct InputState
	{
		State current;
		State previous;
	};
	InputState* state = nullptr;
}

std::unique_ptr<InputManager> InputManager::instance_ = nullptr;

void InputManager::Init()
{
	if (!instance_)
	{
		instance_.reset(new InputManager);
		state = new InputState;
	}
}

void InputManager::Shutdown()
{
	if (instance_)
	{
		instance_.reset();
		delete state;
	}
}

InputManager& InputManager::GetInstance()
{
	if (!instance_)
	{
		DEMO_LOG(Error, "InputManager can't get the handle before Init!");
	}
	return *instance_;
}

void InputManager::update(double _deltatime)
{
	state->previous = state->current;
}

bool InputManager::GetIsKeyDown(KEY key)
{
	return state->current.keys[key] == true;
}

bool InputManager::GetIsKeyUp(KEY key)
{
	return state->current.keys[key] == false;
}

bool InputManager::GetWasKeyDown(KEY key)
{
	return state->previous.keys[key] == true;
}

bool InputManager::GetWasKeyUp(KEY key)
{
	return state->previous.keys[key] == false;
}

bool InputManager::GetIsButtonDown(BUTTON button)
{
	return state->current.buttons[button] == true;
}

bool InputManager::GetIsButtonUp(BUTTON button)
{
	return state->current.buttons[button] == false;
}

bool InputManager::GetWasButtonDown(BUTTON button)
{
	return state->previous.buttons[button] == true;
}

bool InputManager::GetWasButtonUp(BUTTON button)
{
	return state->previous.buttons[button] == false;
}

void InputManager::GetMousePos(int& x, int& y)
{
	x = state->current.x;
	y = state->current.y;
}

void InputManager::GetPrevMousePos(int& x, int& y)
{
	x = state->previous.x;
	y = state->previous.y;
}

void InputManager::ProcessKey(KEY key, bool pressed)
{
	if (state->current.keys[key] != pressed)
	{
		state->current.keys[key] = pressed;
		if (key == KEY_LALT)
		{
			DEMO_LOG(Info, std::format("Left alt {}.", pressed ? "pressed" : "released"));
		}
		else if (key == KEY_RALT)
		{
			DEMO_LOG(Info, std::format("Right alt {}.", pressed ? "pressed" : "released"));
		}

		if (key == KEY_LCONTROL)
		{
			DEMO_LOG(Info, std::format("Left ctrl {}.", pressed ? "pressed" : "released"));
		}
		else if (key == KEY_RCONTROL)
		{
			DEMO_LOG(Info, std::format("Right ctrl {}.", pressed ? "pressed" : "released"));
		}

		if (key == KEY_LSHIFT)
		{
			DEMO_LOG(Info, std::format("Left shift {}.", pressed ? "pressed" : "released"));
		}
		else if (key == KEY_RSHIFT)
		{
			DEMO_LOG(Info, std::format("Right shift {}.", pressed ? "pressed" : "released"));
		}

		// Fire off an event for immediate processing.
		EventContext context;
		context.data.u16[0] = key;
		EventManager::GetInstance().Fire(pressed ? EVENTCODE::KEY_PRESSED : EVENTCODE::KEY_RELEASED, 0, context);
	}
}

void InputManager::ProcessButton(BUTTON button, bool pressed)
{
	if (state->current.buttons[button] != pressed)
	{
		state->current.buttons[button] = pressed;

		// Fire the event.
		EventContext context;
		context.data.u16[0] = button;
		EventManager::GetInstance().Fire(pressed ? EVENTCODE::BUTTON_PRESSED : EVENTCODE::BUTTON_RELEASED, 0, context);
	}
}

void InputManager::ProcessMouseMove(short x, short y)
{
	// Only process if actually different
	if (state->current.x != x || state->current.y != y)
	{
		//DEMO_LOG(Info, std::format("Mouse pos: {}, {}", x, y));
		state->current.x = x;
		state->current.y = y;
		EventContext context;
		context.data.u16[0] = x;
		context.data.u16[1] = y;
		EventManager::GetInstance().Fire(EVENTCODE::MOUSE_MOVED, 0, context);
	}
}

void InputManager::ProcessMouseWheel(signed char z_delta)
{
	EventContext context;
	//DEMO_LOG(Info, std::format("Mouse scroll: {}.", z_delta));
	context.data.i8[0] = z_delta;
	EventManager::GetInstance().Fire(EVENTCODE::MOUSE_WHEEL, 0, context);
}
