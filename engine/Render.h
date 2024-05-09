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
	std::unique_ptr<Buffer> hostVertexbuffer;
	std::unique_ptr<Buffer> deviceVertexbuffer;
	std::vector<std::unique_ptr<Buffer>> hostUniformbuffers;
	std::vector<std::unique_ptr<Buffer>> deviceUniformbuffers;
	vk::DescriptorPool descriptorPool;
	std::vector<vk::DescriptorSet> sets;
	int current_frame;

	void createVertexBuffer();
	void createUniformBuffer();
	void vertexData();
	void uniformData();
	void copyBuffer(vk::Buffer& src, vk::Buffer& dst, size_t size, size_t srcoffset, size_t dstoffset);
	void createDescriptorPool();
	void allocateSets();
	void updateSets();
};

class ImGuiRenderer
{
public:
	ImGuiRenderer();
	~ImGuiRenderer();
};