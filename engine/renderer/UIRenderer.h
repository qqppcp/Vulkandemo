#pragma once
#include <vulkan/vulkan.hpp>

class ImGuiRenderer
{
public:
	ImGuiRenderer();
	~ImGuiRenderer();
	void render(float delta_time = 0) {};

private:
	vk::Device m_device;
	vk::Image m_fontImage;
	vk::DeviceMemory m_memory;
	vk::Image m_fontImageView;
	vk::CommandPool cmdpool;
	vk::DescriptorPool descriptorPool;
};