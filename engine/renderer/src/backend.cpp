#include "backend.h"

#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

#include "window.h"
#include "Context.h"
#include "ShaderPool.h"
#include "program.h"
#include "render_process.h"
#include "Render.h"
#include "UIRenderer.h"
#include "CommandBuffer.h"
#include "log.h"


void VulkanBackend::Init()
{
	Context::InitContext();
	Context::GetInstance().InitSwapchain();

	ShaderPool::Initialize();
}

void VulkanBackend::Quit()
{
	ShaderPool::Quit();
	Context::GetInstance().DestroySwapchain();
	Context::Quit();
}

void VulkanBackend::OnResized(short width, short height)
{
}

bool VulkanBackend::BeginFrame(double _deltatime, vk::CommandBuffer& buffer, vk::Fence& cmdbufAvaliableFence, vk::Semaphore& imageAvaliable)
{
	auto& swapchain = Context::GetInstance().swapchain;
	auto device = Context::GetInstance().device;

	if (device.waitForFences(cmdbufAvaliableFence, true, std::numeric_limits<uint64_t>::max()) != vk::Result::eSuccess)
		DEMO_LOG(Error, "wait for fence failed.");

	device.resetFences(cmdbufAvaliableFence);

	auto result = device.acquireNextImageKHR(swapchain->swapchain, std::numeric_limits<uint64_t>::max(), imageAvaliable);
	if (result.result != vk::Result::eSuccess)
		DEMO_LOG(Error, "can't get next image.");
	//// Start the Dear ImGui frame
	//ImGui_ImplVulkan_NewFrame();
	//ImGui_ImplGlfw_NewFrame();
	//ImGui::NewFrame();
	Context::GetInstance().image_index = result.value;
	buffer.reset();
	vk::CommandBufferBeginInfo cmdBI;
	cmdBI.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	buffer.begin(cmdBI);
	return true;
}

bool VulkanBackend::EndFrame(double _deltatime, vk::CommandBuffer& buffer, vk::Fence& cmdbufAvaliableFence, vk::Semaphore& imageAvaliable, vk::Semaphore& imageDrawFinish)
{
	buffer.end();

	vk::SubmitInfo submitInfo;
	vk::PipelineStageFlags waitMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	submitInfo.setCommandBuffers(buffer)
		.setPWaitDstStageMask(&waitMask)
		.setWaitSemaphores(imageAvaliable)
		.setSignalSemaphores(imageDrawFinish);
	Context::GetInstance().graphicsQueue.submit(submitInfo, cmdbufAvaliableFence);

	Context::GetInstance().swapchain->Present(imageDrawFinish, Context::GetInstance().image_index);
	Context::GetInstance().current_frame = (Context::GetInstance().current_frame + 1) % Context::GetInstance().swapchain->info.imageCount;
	return true;
}

bool VulkanBackend::BeginRenderpass()
{
	return false;
}

bool VulkanBackend::EndRenderpass()
{
	return false;
}


void VulkanBackend::InitImGui(vk::DescriptorPool& descriptorPool, vk::RenderPass& renderPass)
{
}

void VulkanBackend::CleanImGui()
{
}
