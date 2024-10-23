#pragma once
#include <vulkan/vulkan.hpp>
#include <vector>

class CommandManager
{
public:
	static std::vector<vk::CommandBuffer> Allocate(vk::CommandPool& pool, int count, bool is_primary = true);
	static void Free(vk::CommandPool& pool, std::vector<vk::CommandBuffer>& buffers);
	static void Free(vk::CommandPool& pool, vk::CommandBuffer& buffer);
	static void Reset(vk::CommandBuffer& buffer);
	static void Begin(vk::CommandBuffer& buffer, bool is_single_use = true, bool is_renderpass_continue = false, bool is_simulaneous_use = false);
	static void End(vk::CommandBuffer& buffer);
	static vk::CommandBuffer BeginSingle(vk::CommandPool& pool);
	static void EndSingle(vk::CommandPool& pool, vk::CommandBuffer& buffer, vk::Queue& queue);
};