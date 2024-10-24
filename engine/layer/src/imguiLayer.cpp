#include "imguiLayer.h"

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

#include "log.h"
#include "window.h"
#include "Context.h"
#include "CommandBuffer.h"
#include "render_process.h"

namespace
{
	vk::DescriptorPool descriptorPool;
	std::vector<vk::Framebuffer> framebuffers;
	void check_vk_result(VkResult err)
	{
		if (err == 0)
			return;
		DEMO_LOG(Error, std::format("[vulkan] Error: VkResult = {}", (int)err));
		if (err < 0)
			abort();
	}
}

ImGuiLayer::ImGuiLayer()
{
	vk::DescriptorPoolCreateInfo poolCI;
	vk::DescriptorPoolSize poolsize[2];
	poolsize[0].setDescriptorCount(50)
		.setType(vk::DescriptorType::eUniformBuffer);
	poolsize[1].setDescriptorCount(50)
		.setType(vk::DescriptorType::eCombinedImageSampler);
	poolCI.setMaxSets(100)
		.setPoolSizes(poolsize);
	descriptorPool = Context::GetInstance().device.createDescriptorPool(poolCI);
	renderPass.reset(new RenderPass(std::vector<vk::Format>{Context::GetInstance().swapchain->info.surfaceFormat.format},
		std::vector<vk::ImageLayout>{vk::ImageLayout::eUndefined},
		std::vector<vk::ImageLayout>{vk::ImageLayout::ePresentSrcKHR },
		std::vector<vk::AttachmentLoadOp>{vk::AttachmentLoadOp::eDontCare},
		std::vector<vk::AttachmentStoreOp>{vk::AttachmentStoreOp::eStore},
		vk::PipelineBindPoint::eGraphics, {}, UINT32_MAX));
	for (int i = 0; i < Context::GetInstance().swapchain->imageviews.size(); i++)
	{
		auto [width, height] = GetWindowSize();
		std::vector<vk::ImageView> attachments{ Context::GetInstance().swapchain->imageviews[i] };
		vk::FramebufferCreateInfo framebufferCI;
		framebufferCI.setAttachments(attachments)
			.setLayers(1)
			.setRenderPass(renderPass->vkRenderPass())
			.setWidth(width)
			.setHeight(height);
		framebuffers.emplace_back(Context::GetInstance().device.createFramebuffer(framebufferCI));
	}
	OnAttach();
}

ImGuiLayer::~ImGuiLayer()
{
	OnDetach();
	for (auto framebuffer : framebuffers)
	{
		Context::GetInstance().device.destroyFramebuffer(framebuffer);
	}
	renderPass.reset();
	Context::GetInstance().device.destroyDescriptorPool(descriptorPool);
}

void ImGuiLayer::OnRender()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	imguistate.runUI();
	auto current_frame = Context::GetInstance().current_frame;
	auto& cmdbufs = Context::GetInstance().cmdbufs;
	auto [width, height] = GetWindowSize();
	vk::RenderPassBeginInfo renderPassBI;

	renderPassBI.setRenderPass(renderPass->vkRenderPass())
		.setFramebuffer(framebuffers[Context::GetInstance().image_index])
		.setRenderArea(VkRect2D({ 0,0 }, { width, height }));
	cmdbufs[current_frame].beginRenderPass(renderPassBI, vk::SubpassContents::eInline);
	cmdbufs[current_frame].setViewport(0, { vk::Viewport{ 0, 0, (float)width, (float)height, 0.0f, 1.0f } });
	cmdbufs[current_frame].setScissor(0, { vk::Rect2D{vk::Offset2D{0, 0}, Context::GetInstance().swapchain->info.imageExtent} });
	ImGui::Render();
	ImDrawData* draw_data = ImGui::GetDrawData();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdbufs[current_frame]);
	cmdbufs[current_frame].endRenderPass();
}

void ImGuiLayer::OnAttach()
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
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
	io.Fonts->AddFontDefault();
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

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForVulkan(GetWindowHandle(), true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = Context::GetInstance().instance;
	init_info.PhysicalDevice = Context::GetInstance().physicaldevice;
	init_info.Device = Context::GetInstance().device;
	init_info.QueueFamily = Context::GetInstance().queueFamileInfo.graphicsFamilyIndex.value();
	init_info.Queue = Context::GetInstance().graphicsQueue;
	init_info.PipelineCache = VK_NULL_HANDLE;
	init_info.DescriptorPool = descriptorPool;
	init_info.Subpass = 0;
	init_info.MinImageCount = 2;
	init_info.ImageCount = Context::GetInstance().swapchain->info.imageCount;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.Allocator = nullptr;
	init_info.CheckVkResultFn = check_vk_result;
	ImGui_ImplVulkan_Init(&init_info, renderPass->vkRenderPass());

	auto cmdbuf = CommandManager::BeginSingle(Context::GetInstance().graphicsCmdPool);
	ImGui_ImplVulkan_CreateFontsTexture(cmdbuf);
	CommandManager::EndSingle(Context::GetInstance().graphicsCmdPool, cmdbuf, Context::GetInstance().graphicsQueue);
	ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void ImGuiLayer::OnDetach()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void ImGuiLayer::addUI(ImGuiBase* pUI)
{
	imguistate.addUI(pUI);
}
