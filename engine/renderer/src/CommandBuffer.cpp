#include "CommandBuffer.h"

#include "Context.h"

std::vector<vk::CommandBuffer> CommandManager::Allocate(vk::CommandPool& pool, int count, bool is_primary)
{
	auto device = Context::GetInstance().device;
	vk::CommandBufferAllocateInfo cmdbufAI;
	cmdbufAI.setCommandBufferCount(count)
		.setCommandPool(pool)
		.setLevel(is_primary ? vk::CommandBufferLevel::ePrimary : vk::CommandBufferLevel::eSecondary);
	auto bufs = device.allocateCommandBuffers(cmdbufAI);
	return bufs;
}

void CommandManager::Free(vk::CommandPool& pool, std::vector<vk::CommandBuffer>& buffers)
{
	auto device = Context::GetInstance().device;
	device.freeCommandBuffers(pool, buffers);
}

void CommandManager::Free(vk::CommandPool& pool, vk::CommandBuffer& buffer)
{
	std::vector<vk::CommandBuffer> buffers{ buffer };
	Free(pool, buffers);
}

void CommandManager::Reset(vk::CommandBuffer& buffer)
{
	buffer.reset();
}

void CommandManager::Begin(vk::CommandBuffer& buffer, bool is_single_use, bool is_renderpass_continue, bool is_simulaneous_use)
{
	auto device = Context::GetInstance().device;
	vk::CommandBufferBeginInfo cmdbufBI;
	vk::CommandBufferUsageFlags flag;
	if (is_single_use)
		flag |= vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	if (is_renderpass_continue)
		flag |= vk::CommandBufferUsageFlagBits::eRenderPassContinue;
	if (is_simulaneous_use)
		flag |= vk::CommandBufferUsageFlagBits::eSimultaneousUse;
	cmdbufBI.setFlags(flag);
	buffer.begin(cmdbufBI);
}

void CommandManager::End(vk::CommandBuffer& buffer)
{
	buffer.end();
}

vk::CommandBuffer CommandManager::BeginSingle(vk::CommandPool& pool)
{
	auto cmdbuf = Allocate(pool, 1, true)[0];
	Begin(cmdbuf);
	return cmdbuf;
}

void CommandManager::EndSingle(vk::CommandPool& pool, vk::CommandBuffer& buffer, vk::Queue& queue)
{
	End(buffer);
	vk::SubmitInfo submitInfo;
	submitInfo.setCommandBuffers(buffer);
	queue.submit(submitInfo);
	queue.waitIdle();
	Free(pool, buffer);
}
