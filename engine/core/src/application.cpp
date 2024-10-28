#include "application.h"

#include <format>

#include <imgui_impl_vulkan.h>

#include "window.h"
#include "backend.h"
#include "input.h"
#include "event.h"
#include "mesh.h"
#include "log.h"
#include "Context.h"
#include "FrameTimer.h"
#include "program.h"
#include "render_process.h"
#include "ImGuiState.h"
#include "termination.h"
#include "FrameTimeInfo.h"
#include "CommandBuffer.h"
#include "geometry.h"
#include "camera.h"
#include "Texture.h"
#include "Sampler.h"
#include "Pipeline.h"
#include "modelforwardLayer.h"
#include "skyboxLayer.h"
#include "imguiLayer.h"

bool app_on_event(unsigned short code, void* sender, void* listener_inst, EventContext context);
bool app_on_key(unsigned short code, void* sender, void* listener_inst, EventContext context);
bool app_on_resized(unsigned short code, void* sender, void* listener_inst, EventContext context);

namespace
{
	struct AppState
	{
		bool running;
		bool suspended;
		int width;
		int height;
		FrameTimer timer;
	};
	AppState state;
	std::string shaderPath = R"(assets\shaders\)";
	std::string texturePath = R"(assets\textures\)";
	std::string modelPath = R"(assets\models\)";
	std::shared_ptr<ModelForwardLayer> layer;
	std::shared_ptr<SkyboxLayer> skyboxLayer;
	std::shared_ptr<ImGuiLayer> uiLayer;
	float rot = 180;
}


Application::Application(int width, int height, std::string name)
{
	{
		state.width = width;
		state.height = height;
		state.running = true;
		state.suspended = false;
		state.timer.newFrame();
	}
	EventManager::Init();
	InputManager::Init();
	GeometryManager::Init();
	EventManager::GetInstance().Register(EVENTCODE::APPLICATION_QUIT, nullptr, app_on_event);
	EventManager::GetInstance().Register(EVENTCODE::KEY_PRESSED, nullptr, app_on_key);
	EventManager::GetInstance().Register(EVENTCODE::KEY_RELEASED, nullptr, app_on_key);
	EventManager::GetInstance().Register(EVENTCODE::RESIZED, nullptr, app_on_resized);
	CameraManager::init({ 0, 0, 2 });
	CreateWindow(width, height, name.data());
	VulkanBackend::Init();
	layer.reset(new ModelForwardLayer(modelPath + "nanosuit_reflect/nanosuit.obj"));
	skyboxLayer.reset(new SkyboxLayer(texturePath + "skybox.hdr"));
	uiLayer.reset(new ImGuiLayer());
	uiLayer->addUI(GetTermination());
	uiLayer->addUI(new ImGuiFrameTimeInfo(&state.timer));
	uiLayer->addUI(layer.get());

}

Application::~Application()
{
	auto device = Context::GetInstance().device;
	uiLayer.reset();
	skyboxLayer.reset();
	layer.reset();
	VulkanBackend::Quit();
	DestroyWindow();
	EventManager::GetInstance().Unregister(EVENTCODE::APPLICATION_QUIT, nullptr, app_on_event);
	EventManager::GetInstance().Unregister(EVENTCODE::KEY_PRESSED, nullptr, app_on_key);
	EventManager::GetInstance().Unregister(EVENTCODE::KEY_RELEASED, nullptr, app_on_key);
	EventManager::GetInstance().Unregister(EVENTCODE::RESIZED, nullptr, app_on_resized);
	GeometryManager::Quit();
	InputManager::Shutdown();
	EventManager::Shutdown();
}

void Application::run()
{
	std::vector<vk::Semaphore> imageAvaliables;
	std::vector<vk::Semaphore> imageDrawFinishs;
	std::vector<vk::Fence> cmdbufAvaliableFences;
	auto& device = Context::GetInstance().device;
	uint32_t maxFlight = Context::GetInstance().swapchain->info.imageCount;
	vk::SemaphoreCreateInfo semaphoreCI;
	vk::FenceCreateInfo fenceCI;
	imageAvaliables.resize(maxFlight);
	imageDrawFinishs.resize(maxFlight);
	cmdbufAvaliableFences.resize(maxFlight);
	for (size_t i = 0; i < maxFlight; i++)
	{
		imageAvaliables[i] = device.createSemaphore(semaphoreCI);
		imageDrawFinishs[i] = device.createSemaphore(semaphoreCI);
		fenceCI.setFlags(vk::FenceCreateFlagBits::eSignaled);
		cmdbufAvaliableFences[i] = device.createFence(fenceCI);
	}

	while (!WindowShouleClose())
	{
		state.timer.newFrame();
		auto deltatime = state.timer.lastFrameTime<std::chrono::milliseconds>();
		auto current_frame = Context::GetInstance().current_frame;
		auto& cmdbufs = Context::GetInstance().cmdbufs;
		{
			ProcessInput(*CameraManager::mainCamera, deltatime.count() / 1000.0);
			layer->OnUpdate(deltatime.count() / 1000.0);
		}
		VulkanBackend::BeginFrame(deltatime.count() / 1000.0, cmdbufs[current_frame], cmdbufAvaliableFences[current_frame], imageAvaliables[current_frame]);
		layer->OnRender();
		skyboxLayer->OnRender();
		uiLayer->OnRender();

		VulkanBackend::EndFrame(deltatime.count() / 1000.0, cmdbufs[current_frame], cmdbufAvaliableFences[current_frame], imageAvaliables[current_frame], imageDrawFinishs[current_frame]);
		WindowEventProcessing();
	}

	device.waitIdle();
	for (size_t i = 0; i < maxFlight; i++)
	{
		device.destroyFence(cmdbufAvaliableFences[i]);
		device.destroySemaphore(imageAvaliables[i]);
		device.destroySemaphore(imageDrawFinishs[i]);
	}
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
			//DEMO_LOG(Info, std::format("{} key pressed in window.", (char)key_code));
		}
	}
	else if (code == EVENTCODE::KEY_RELEASED)
	{
		auto key_code = context.data.u16[0];
		//DEMO_LOG(Info, std::format("{} key released in window.", (char)key_code));
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