#pragma once
#include <vulkan/vulkan.hpp>
#include "Buffer.h"

class VulkanBackend
{
public:
	static void Init();
	static void Quit();
	static void OnResized(short width, short height);
	static bool BeginFrame(double _deltatime, vk::CommandBuffer& buffer, vk::Fence& cmdbufAvaliableFence, vk::Semaphore& imageAvaliable);
	static bool EndFrame(double _deltatime, vk::CommandBuffer& buffer, vk::Fence& cmdbufAvaliableFence, vk::Semaphore& imageAvaliable, vk::Semaphore& imageDrawFinish);
	static bool BeginRenderpass();
	static bool EndRenderpass();
	static void CreateRendertarget();
	static void InitImGui(vk::DescriptorPool& descriptorPool, vk::RenderPass& renderPass);
	static void CleanImGui();
};
