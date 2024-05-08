#include "Context.h"
#include "GLFW/glfw3.h"
#include "log.h"
#include "Render.h"
#include "window.h"

#include "Vertex.h"

#include "imgui/imgui.h"

int maxFlight;
Vertex vertices[3] = {
	Vertex{-0.5,  0.5, 1.0, 0.0, 0.0, 1.0},
	Vertex{ 0.5,  0.5, 0.0, 1.0, 0.0, 1.0},
	Vertex{ 0  , -0.5, 0.0, 0.0, 1.0, 1.0}
};
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
	vertexbuffer.reset(new Buffer(sizeof(vertices), vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
	void* ptr = device.mapMemory(vertexbuffer->memory, 0, sizeof(vertices));
	memcpy(ptr, vertices, sizeof(vertices));
	device.unmapMemory(vertexbuffer->memory);
}

Renderer::~Renderer()
{
	auto& device = Context::GetInstance().device;
	device.waitIdle();
	vertexbuffer.reset();
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
		vk::DeviceSize offset = 0;
		cmdbufs[current_frame].bindVertexBuffers(0, vertexbuffer->buffer, offset);
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


ImGuiRenderer::ImGuiRenderer()
{
	ImGui::CreateContext();
	// Color Schme
	ImGuiStyle& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.0f, 0.0f, 0.0f, 0.1f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.8f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.8f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
	style.Colors[ImGuiCol_Button] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);

	// Dimensions
	ImGuiIO& io = ImGui::GetIO();
	io.FontGlobalScale = 1.0f;
	io.DisplayFramebufferScale = ImVec2(1.f, 1.f);

	// Keymap [GLFW]
	io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
	io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
	io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
	io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
	io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
	io.KeyMap[ImGuiKey_Insert] = GLFW_KEY_INSERT;
	io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
	io.KeyMap[ImGuiKey_Space] = GLFW_KEY_SPACE;
	io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
	io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
	io.KeyMap[ImGuiKey_KeyPadEnter] = GLFW_KEY_KP_ENTER;
	io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
	io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
	io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
	io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
	io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
	io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;
}

ImGuiRenderer::~ImGuiRenderer()
{
	ImGui::DestroyContext();
}