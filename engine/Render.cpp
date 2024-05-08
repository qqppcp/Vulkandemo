#include "Context.h"
#include "log.h"
#include "Render.h"
#include "window.h"

int maxFlight;
Renderer::Renderer(RenderProcess* process, std::vector<vk::Framebuffer> framebuffers)
	: process(process), framebuffers(framebuffers), current_frame(0)
{
	auto& device = Context::GetInstance().device;
	maxFlight = Context::GetInstance().swapchain->info.imageCount;
	vk::CommandPoolCreateInfo poolCI;
	poolCI.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
	cmdPool = device.createCommandPool(poolCI);
	vk::CommandBufferAllocateInfo bufAI;
	bufAI.setCommandPool(cmdPool)
		.setCommandBufferCount(maxFlight)
		.setLevel(vk::CommandBufferLevel::ePrimary);
	cmdbufs = device.allocateCommandBuffers(bufAI);
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

}

Renderer::~Renderer()
{
	auto& device = Context::GetInstance().device;
	device.waitIdle();
	for (size_t i = 0; i < maxFlight; i++)
	{
		device.destroyFence(cmdbufAvaliableFences[i]);
		device.destroySemaphore(imageAvaliables[i]);
		device.destroySemaphore(imageDrawFinishs[i]);
	}
	device.freeCommandBuffers(cmdPool, cmdbufs);
	device.destroyCommandPool(cmdPool);
}

void Renderer::render(float delta_time)
{
	auto& swapchain = Context::GetInstance().swapchain;
	auto device = Context::GetInstance().device;

	if (device.waitForFences(cmdbufAvaliableFences[current_frame], true, std::numeric_limits<uint64_t>::max()) != vk::Result::eSuccess)
		DEMO_LOG(Error, "wait for fence failed.");

	device.resetFences(cmdbufAvaliableFences[current_frame]);

	auto result = device.acquireNextImageKHR(swapchain->swapchain, std::numeric_limits<uint64_t>::max(), imageAvaliables[current_frame]);
	if (result.result != vk::Result::eSuccess)
		DEMO_LOG(Error, "can't get next image.");
	auto imageIndex = result.value;

	cmdbufs[current_frame].reset();
	vk::CommandBufferBeginInfo cmdBI;
	cmdBI.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	cmdbufs[current_frame].begin(cmdBI);
	{
		cmdbufs[current_frame].bindPipeline(vk::PipelineBindPoint::eGraphics, process->pipeline);
		vk::RenderPassBeginInfo renderPassBI;
		auto [width, height] = GetWindowSize();
		vk::ClearValue clear;
		clear.setColor({ 0.05f, 0.05f, 0.05f, 1.0f });
		renderPassBI.setRenderPass(process->renderPass)
			.setFramebuffer(framebuffers[imageIndex])
			.setRenderArea(VkRect2D({ 0,0 }, { width, height }))
			.setClearValues(clear);
		cmdbufs[current_frame].beginRenderPass(renderPassBI, {});
		cmdbufs[current_frame].draw(3, 1, 0, 0);
	}
	cmdbufs[current_frame].endRenderPass();
	cmdbufs[current_frame].end();

	vk::SubmitInfo submitInfo;
	vk::PipelineStageFlags waitMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	submitInfo.setCommandBuffers(cmdbufs[current_frame])
		.setPWaitDstStageMask(&waitMask)
		.setWaitSemaphores(imageAvaliables[current_frame])
		.setSignalSemaphores(imageDrawFinishs[current_frame]);
	Context::GetInstance().graphicsQueue.submit(submitInfo, cmdbufAvaliableFences[current_frame]);


	vk::PresentInfoKHR presentInfo;
	presentInfo.setImageIndices(imageIndex)
		.setSwapchains(swapchain->swapchain)
		.setWaitSemaphores(imageDrawFinishs[current_frame]);
	if(Context::GetInstance().presentQueue.presentKHR(presentInfo) != vk::Result::eSuccess)
		DEMO_LOG(Error, "present image failed.");

	current_frame = (current_frame + 1) % maxFlight;
}
