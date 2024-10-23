#include "Context.h"
#include "GLFW/glfw3.h"
#include "log.h"
#include "Render.h"
#include "window.h"
#include "Vertex.h"
#include "Uniform.h"
#include "CommandBuffer.h"

#include "imgui/imgui.h"
#include "imgui_impl_vulkan.h"
#include "imgui_impl_glfw.h"

#include "ImGuiState.h"
#include "termination.h"
#include "FrameTimeInfo.h"

#include "mesh.h"
std::string modelPath = R"(assets\models\)";

static void check_vk_result(VkResult err)
{
	if (err == 0)
		return;
	DEMO_LOG(Error, std::format("[vulkan] Error: VkResult = {}", (int)err));
	if (err < 0)
		abort();
}

int maxFlight;
Vertex vertices[3] = {
	Vertex{-0.5,  0.5, 1.0, 0.0, 0.0, 1.0},
	Vertex{ 0.5,  0.5, 0.0, 1.0, 0.0, 1.0},
	Vertex{ 0  , -0.5, 0.0, 0.0, 1.0, 1.0}
};
Uniform uniform;

Renderer::Renderer(RenderProcess* process, std::vector<vk::Framebuffer> framebuffers)
	: process(process), framebuffers(framebuffers), current_frame(0)
{
	uniform.color = { 0.6f, 0.3f, 0.4f, 1.0f };
	auto& device = Context::GetInstance().device;
	maxFlight = Context::GetInstance().swapchain->info.imageCount;
	cmdbufs = CommandManager::Allocate(Context::GetInstance().graphicsCmdPool, maxFlight, true);
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
	createVertexBuffer();
	createUniformBuffer();
	vertexData();
	uniformData();
	createDescriptorPool();
	allocateSets();
	updateSets();
	initImGui();
	imguistate.reset(new ImGuiState);
	imguistate->addUI(GetTermination());
	imguistate->addUI(new ImGuiFrameTimeInfo(&timer));
	imguistate->addUI(&uniform);
	Mesh mesh;
	mesh.load(modelPath + "nanosuit_reflect/nanosuit.obj");
}

Renderer::~Renderer()
{
	auto& device = Context::GetInstance().device;
	device.waitIdle();
	imguistate.reset();
	cleanImGui();
	device.destroyDescriptorPool(descriptorPool);
	hostVertexbuffer.reset();
	deviceVertexbuffer.reset();
	for (int i = 0; i < hostUniformbuffers.size(); i++)
	{
		hostUniformbuffers[i].reset();
		deviceUniformbuffers[i].reset();
	}
	for (size_t i = 0; i < maxFlight; i++)
	{
		device.destroyFence(cmdbufAvaliableFences[i]);
		device.destroySemaphore(imageAvaliables[i]);
		device.destroySemaphore(imageDrawFinishs[i]);
	}
	CommandManager::Free(Context::GetInstance().graphicsCmdPool, cmdbufs);
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
	// Start the Dear ImGui frame
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	timer.newFrame();
	imguistate->runUI();
	auto imageIndex = result.value;
	{
		float time = static_cast<float>(glfwGetTime());
		//uniform.color = glm::vec4{ abs(sin(time)), abs(sin(0.314 * time)) , abs(cos(time)), 1.0f };
		auto& buffer = hostUniformbuffers[current_frame];
		void* ptr = device.mapMemory(buffer->memory, 0, sizeof(uniform.color));
		memcpy(ptr, &uniform.color[0], sizeof(uniform.color));
		device.unmapMemory(buffer->memory);
		copyBuffer(buffer->buffer, deviceUniformbuffers[current_frame]->buffer, buffer->size, 0, 0);
	}
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
		cmdbufs[current_frame].bindVertexBuffers(0, deviceVertexbuffer->buffer, offset);
		cmdbufs[current_frame].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, process->layout, 0, sets[current_frame], {});
		cmdbufs[current_frame].beginRenderPass(renderPassBI, {});
		cmdbufs[current_frame].draw(3, 1, 0, 0);

		// Rendering
		ImGui::Render();
		ImDrawData* draw_data = ImGui::GetDrawData();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdbufs[current_frame]);
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

	swapchain->Present(imageDrawFinishs[current_frame], imageIndex);
	current_frame = (current_frame + 1) % maxFlight;
}

void Renderer::createVertexBuffer()
{
	hostVertexbuffer.reset(new Buffer(sizeof(vertices), 
		vk::BufferUsageFlagBits::eTransferSrc, 
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
	deviceVertexbuffer.reset(new Buffer(sizeof(vertices),
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal));
}
void Renderer::createUniformBuffer()
{
	hostUniformbuffers.resize(maxFlight);
	deviceUniformbuffers.resize(maxFlight);

	for (auto& buffer : hostUniformbuffers)
	{
		buffer.reset(new Buffer(sizeof(uniform.color),
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
	}
	for (auto& buffer : deviceUniformbuffers)
	{
		buffer.reset(new Buffer(sizeof(uniform.color),
			vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer,
			vk::MemoryPropertyFlagBits::eDeviceLocal));
	}
}
void Renderer::vertexData()
{
	auto device = Context::GetInstance().device;
	void* ptr = device.mapMemory(hostVertexbuffer->memory, 0, sizeof(vertices));
	memcpy(ptr, vertices, sizeof(vertices));
	device.unmapMemory(hostVertexbuffer->memory);
	copyBuffer(hostVertexbuffer->buffer, deviceVertexbuffer->buffer, hostVertexbuffer->size, 0, 0);
}
void Renderer::uniformData()
{
	auto device = Context::GetInstance().device;
	for (size_t i = 0; i < hostUniformbuffers.size(); i++)
	{
		auto& buffer = hostUniformbuffers[i];
		void* ptr = device.mapMemory(buffer->memory, 0, sizeof(uniform.color));
		memcpy(ptr, &uniform.color[0], sizeof(uniform.color));
		device.unmapMemory(buffer->memory);
		copyBuffer(buffer->buffer, deviceUniformbuffers[i]->buffer, buffer->size, 0, 0);
	}
}
void Renderer::copyBuffer(vk::Buffer& src, vk::Buffer& dst, size_t size, size_t srcoffset, size_t dstoffset)
{
	auto cmdbuf = CommandManager::BeginSingle(Context::GetInstance().graphicsCmdPool);
	vk::BufferCopy region;
	region.setSize(size)
		.setSrcOffset(0)
		.setDstOffset(0);
	cmdbuf.copyBuffer(src, dst, region);
	CommandManager::EndSingle(Context::GetInstance().graphicsCmdPool, cmdbuf, Context::GetInstance().graphicsQueue);
}
void Renderer::createDescriptorPool()
{
	vk::DescriptorPoolCreateInfo poolCI;
	vk::DescriptorPoolSize poolsize[2];
	poolsize[0].setDescriptorCount(maxFlight)
		.setType(vk::DescriptorType::eUniformBuffer);
	poolsize[1].setDescriptorCount(maxFlight)
		.setType(vk::DescriptorType::eCombinedImageSampler);
	poolCI.setMaxSets(maxFlight + maxFlight)
		.setPoolSizes(poolsize);
	descriptorPool = Context::GetInstance().device.createDescriptorPool(poolCI);
}
void Renderer::allocateSets()
{
	std::vector<vk::DescriptorSetLayout> layouts(maxFlight, process->setLayout);
	vk::DescriptorSetAllocateInfo setAI;
	setAI.setDescriptorPool(descriptorPool)
		.setDescriptorSetCount(maxFlight)
		.setSetLayouts(layouts);

	sets = Context::GetInstance().device.allocateDescriptorSets(setAI);
}
void Renderer::updateSets()
{
	for (size_t i = 0; i < sets.size(); i++)
	{
		auto& set = sets[i];
		vk::DescriptorBufferInfo bufferInfo;
		bufferInfo.setBuffer(deviceUniformbuffers[i]->buffer)
			.setOffset(0)
			.setRange(deviceUniformbuffers[i]->size);
		vk::WriteDescriptorSet writer;
		writer.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setBufferInfo(bufferInfo)
			.setDstSet(set)
			.setDstArrayElement(0)
			.setDescriptorCount(1);
		Context::GetInstance().device.updateDescriptorSets(writer, {});
	}
}
void Renderer::initImGui()
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
	init_info.ImageCount = maxFlight;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.Allocator = nullptr;
	init_info.CheckVkResultFn = check_vk_result;
	ImGui_ImplVulkan_Init(&init_info, process->renderPass);

	auto cmdbuf = CommandManager::BeginSingle(Context::GetInstance().graphicsCmdPool);
	ImGui_ImplVulkan_CreateFontsTexture(cmdbuf);
	CommandManager::EndSingle(Context::GetInstance().graphicsCmdPool, cmdbuf, Context::GetInstance().graphicsQueue);
	ImGui_ImplVulkan_DestroyFontUploadObjects();
}
void Renderer::cleanImGui()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}