#pragma once

#include "vulkan/vulkan.hpp"
#include "render_process.h"
#include "Buffer.h"

class Renderer final {
public:
	Renderer(RenderProcess*, std::vector<vk::Framebuffer>);
	~Renderer();

	void render(float delta_time = 0.f);
	vk::CommandPool cmdPool;
	std::vector<vk::CommandBuffer> cmdbufs;
private:
	std::vector<vk::Semaphore> imageAvaliables;
	std::vector<vk::Semaphore> imageDrawFinishs;
	std::vector<vk::Fence> cmdbufAvaliableFences;
	RenderProcess* process;
	std::vector<vk::Framebuffer> framebuffers;
	std::unique_ptr<Buffer> vertexbuffer;
	int current_frame;
};

class ImGuiRenderer
{
public:
	ImGuiRenderer();
	~ImGuiRenderer();
};