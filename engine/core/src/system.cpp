#include "system.h"

#include <format>

#include <imgui_impl_vulkan.h>

#include "window.h"
#include "backend.h"
#include "input.h"
#include "event.h"
#include "log.h"
#include "geometry.h"
#include "Context.h"
#include "Texture.h"

bool app_on_event(unsigned short code, void* sender, void* listener_inst, EventContext context);
bool app_on_key(unsigned short code, void* sender, void* listener_inst, EventContext context);
bool app_on_resized(unsigned short code, void* sender, void* listener_inst, EventContext context);

void SystemManger::Init(uint32_t width, uint32_t height, std::string name)
{
	EventManager::Init();
	InputManager::Init();
	EventManager::GetInstance().Register(EVENTCODE::APPLICATION_QUIT, nullptr, app_on_event);
	EventManager::GetInstance().Register(EVENTCODE::KEY_PRESSED, nullptr, app_on_key);
	EventManager::GetInstance().Register(EVENTCODE::KEY_RELEASED, nullptr, app_on_key);
	EventManager::GetInstance().Register(EVENTCODE::RESIZED, nullptr, app_on_resized);
	CreateWindow(width, height, name.data());
	VulkanBackend::Init();
	GeometryManager::Init();
}

void SystemManger::Shutdown()
{
	TextureManager::Instance().Clear();
	GeometryManager::Quit();
	VulkanBackend::Quit();
	DestroyWindow();
	EventManager::GetInstance().Unregister(EVENTCODE::APPLICATION_QUIT, nullptr, app_on_event);
	EventManager::GetInstance().Unregister(EVENTCODE::KEY_PRESSED, nullptr, app_on_key);
	EventManager::GetInstance().Unregister(EVENTCODE::KEY_RELEASED, nullptr, app_on_key);
	EventManager::GetInstance().Unregister(EVENTCODE::RESIZED, nullptr, app_on_resized);
	InputManager::Shutdown();
	EventManager::Shutdown();
}

/*-----------------------------------------------------------------*/
bool app_on_event(unsigned short code, void* sender, void* listener_inst, EventContext context)
{
	if (code == EVENTCODE::APPLICATION_QUIT)
	{
		return true;
	}
	return false;
}
bool app_on_key(unsigned short code, void* sender, void* listener_inst, EventContext context)
{
	if (code == EVENTCODE::KEY_PRESSED)
	{
		auto key_code = context.data.u16[0];
		if (key_code == KEY_ESCAPE)
		{
			EventContext data = {};
			EventManager::GetInstance().Fire(EVENTCODE::APPLICATION_QUIT, 0, data);
			return true;
		}
		else
		{
			DEMO_LOG(Info, std::format("{} key pressed in window.", (char)key_code));
		}
	}
	else if (code == EVENTCODE::KEY_RELEASED)
	{
		auto key_code = context.data.u16[0];
		DEMO_LOG(Info, std::format("{} key released in window.", (char)key_code));
	}
	return false;
}
bool app_on_resized(unsigned short code, void* sender, void* listener_inst, EventContext context)
{
	if (code == EVENTCODE::RESIZED)
	{
		auto width = context.data.u16[0];
		auto height = context.data.u16[1];
	}
	return false;
}