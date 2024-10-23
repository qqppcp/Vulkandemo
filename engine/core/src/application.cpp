#include "application.h"

#include <format>

#include <imgui_impl_vulkan.h>

#include "window.h"
#include "backend.h"
#include "input.h"
#include "event.h"
#include "mesh.h"
#include "log.h"
#include "Context.h"
#include "FrameTimer.h"
#include "program.h"
#include "render_process.h"
#include "UIRenderer.h"
#include "ImGuiState.h"
#include "termination.h"
#include "FrameTimeInfo.h"
#include "CommandBuffer.h"
#include "Uniform.h"
#include "camera.h"
#include "Texture.h"
#include "Sampler.h"
#include "Pipeline.h"

bool app_on_event(unsigned short code, void* sender, void* listener_inst, EventContext context);
bool app_on_key(unsigned short code, void* sender, void* listener_inst, EventContext context);
bool app_on_resized(unsigned short code, void* sender, void* listener_inst, EventContext context);

namespace
{
	struct AppState
	{
		bool running;
		bool suspended;
		int width;
		int height;
		FrameTimer timer;
	};
	AppState state;
	Mesh mesh;
	std::string shaderPath = R"(assets\shaders\)";
	std::string texturePath = R"(assets\textures\)";
	std::string modelPath = R"(assets\models\)";
	std::unique_ptr<GPUProgram> triangle;
	std::unique_ptr<ImGuiRenderer> imguiRenderer;
	std::vector<vk::Framebuffer> framebuffers;
	Uniform uniform;
	std::shared_ptr<Buffer> vertexBuffer;
	std::shared_ptr<Buffer> indiceBuffer;
	std::shared_ptr<Buffer> materialBuffer;
	std::shared_ptr<Buffer> uniformBuffer;
	std::shared_ptr<Texture> texture;
	std::shared_ptr<Texture> depth;
	std::shared_ptr<Sampler> sampler;
	std::shared_ptr<RenderPass> renderPass;
	std::shared_ptr<Pipeline> pipeline;
	constexpr uint32_t COLOR_SET = 0;
	constexpr uint32_t TEXTURES_AND_SAMPLER_SET = 1;
	constexpr uint32_t VERTEX_INDEX_SET = 2;
	constexpr uint32_t MATERIAL_SET = 3;
	float rot = 180;
}

Camera mainCamera(glm::vec3{0, 0, 2});

Application::Application(int width, int height, std::string name)
{
	{
		state.width = width;
		state.height = height;
		state.running = true;
		state.suspended = false;
		state.timer.newFrame();
	}
	EventManager::Init();
	InputManager::Init();
	EventManager::GetInstance().Register(EVENTCODE::APPLICATION_QUIT, nullptr, app_on_event);
	EventManager::GetInstance().Register(EVENTCODE::KEY_PRESSED, nullptr, app_on_key);
	EventManager::GetInstance().Register(EVENTCODE::KEY_RELEASED, nullptr, app_on_key);
	EventManager::GetInstance().Register(EVENTCODE::RESIZED, nullptr, app_on_resized);
	CreateWindow(width, height, name.data());
	VulkanBackend::Init();

	triangle.reset(new GPUProgram(shaderPath + "triangle.vert.spv", shaderPath + "triangle.frag.spv"));
	auto& swapchian = Context::GetInstance().swapchain;
	auto device = Context::GetInstance().device;

	uniform.color = { 0.6f, 0.3f, 0.4f, 1.0f };
	mesh.loadobj(modelPath + "nanosuit_reflect/nanosuit.obj");

	vertexBuffer.reset(new Buffer(mesh.vertices.size() * sizeof(Vertex), vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal));
	indiceBuffer.reset(new Buffer(mesh.indices.size() * sizeof(std::uint32_t), vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndexBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal));
	materialBuffer.reset(new Buffer(mesh.materials.size() * sizeof(Material), vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal));
	uniformBuffer.reset(new Buffer(sizeof(glm::vec4), vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal));
	UploadBufferData({}, vertexBuffer, mesh.vertices.size() * sizeof(Vertex), mesh.vertices.data());
	UploadBufferData({}, indiceBuffer, mesh.indices.size() * sizeof(std::uint32_t), mesh.indices.data());
	UploadBufferData({}, materialBuffer, mesh.materials.size() * sizeof(Material), mesh.materials.data());
	UploadBufferData({}, uniformBuffer, sizeof(glm::vec4), &uniform.color[0]);
	{
		texture = TextureManager::Instance().Load(texturePath + "role.png");
		sampler.reset(new Sampler(vk::Filter::eLinear, vk::Filter::eLinear,
			vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
			vk::SamplerAddressMode::eRepeat, 10.0f));
		depth = TextureManager::Instance().Create(state.width, state.height, vk::Format::eD24UnormS8Uint);
	}
	{
		//renderPass.reset(new RenderPass(std::vector<vk::Format>{Context::GetInstance().swapchain->info.surfaceFormat.format},
		//	std::vector<vk::ImageLayout>{vk::ImageLayout::eUndefined}, std::vector<vk::ImageLayout>{vk::ImageLayout::ePresentSrcKHR},
		//	std::vector<vk::AttachmentLoadOp>{vk::AttachmentLoadOp::eClear}, std::vector<vk::AttachmentStoreOp>{vk::AttachmentStoreOp::eStore},
		//	vk::PipelineBindPoint::eGraphics, {}, 1));
		renderPass.reset(new RenderPass({ Context::GetInstance().swapchain->texture(0), depth }, {},
			std::vector<vk::AttachmentLoadOp>{vk::AttachmentLoadOp::eClear, vk::AttachmentLoadOp::eClear},
			std::vector<vk::AttachmentStoreOp>{vk::AttachmentStoreOp::eStore, vk::AttachmentStoreOp::eDontCare},
			std::vector<vk::ImageLayout>{vk::ImageLayout::ePresentSrcKHR, vk::ImageLayout::eDepthStencilAttachmentOptimal},
			vk::PipelineBindPoint::eGraphics));
	}
	{
		for (int i = 0; i < swapchian->imageviews.size(); i++)
		{
			std::vector<vk::ImageView> attachments{ swapchian->imageviews[i], depth->view };
			vk::FramebufferCreateInfo framebufferCI;
			framebufferCI.setAttachments(attachments)
				.setLayers(1)
				.setRenderPass(renderPass->vkRenderPass())
				.setWidth(width)
				.setHeight(height);
			framebuffers.emplace_back(device.createFramebuffer(framebufferCI));
		}
	}
	{
		std::vector<Pipeline::SetDescriptor> setLayouts;
		{
			Pipeline::SetDescriptor set;
			set.set = COLOR_SET;
			vk::DescriptorSetLayoutBinding binding;
			binding.setBinding(0)
				.setDescriptorCount(1)
				.setDescriptorType(vk::DescriptorType::eUniformBuffer)
				.setStageFlags(vk::ShaderStageFlagBits::eFragment);
			set.bindings.push_back(binding);
			setLayouts.push_back(set);
		}
		{
			Pipeline::SetDescriptor set;
			set.set = TEXTURES_AND_SAMPLER_SET;
			vk::DescriptorSetLayoutBinding binding;
			binding.setBinding(0)
				.setDescriptorCount(50)
				.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
				.setStageFlags(vk::ShaderStageFlagBits::eFragment);
			set.bindings.push_back(binding);
			setLayouts.push_back(set);
		}
		{
			Pipeline::SetDescriptor set;
			set.set = VERTEX_INDEX_SET;
			vk::DescriptorSetLayoutBinding binding;
			binding.setBinding(0)
				.setDescriptorCount(1)
				.setDescriptorType(vk::DescriptorType::eStorageBuffer)
				.setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
			set.bindings.push_back(binding);
			binding.setBinding(1)
				.setDescriptorCount(1)
				.setDescriptorType(vk::DescriptorType::eStorageBuffer)
				.setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
			set.bindings.push_back(binding);
			setLayouts.push_back(set);
		}
		{
			Pipeline::SetDescriptor set;
			set.set = MATERIAL_SET;
			vk::DescriptorSetLayoutBinding binding;
			binding.setBinding(0)
				.setDescriptorCount(1)
				.setDescriptorType(vk::DescriptorType::eStorageBuffer)
				.setStageFlags(vk::ShaderStageFlagBits::eFragment);
			set.bindings.push_back(binding);
			setLayouts.push_back(set);
		}
		std::vector<vk::PushConstantRange> ranges(2);
		ranges[0].setOffset(0)
			.setSize(sizeof(glm::mat4) * 3)
			.setStageFlags(vk::ShaderStageFlagBits::eVertex);
		ranges[1].setOffset(sizeof(glm::mat4) * 3)
			.setSize(sizeof(glm::vec3))
			.setStageFlags(vk::ShaderStageFlagBits::eFragment);
		const Pipeline::GraphicsPipelineDescriptor gpDesc = {
			.sets = setLayouts,
			.vertexShader = triangle->Vertex,
			.fragmentShader = triangle->Fragment,
			.pushConstants = {ranges},
			.dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor,
				vk::DynamicState::eDepthTestEnable},
			.colorTextureFormats = {Context::GetInstance().swapchain->info.surfaceFormat.format},
			.depthTextureFormat = vk::Format::eD24UnormS8Uint,
			.cullMode = vk::CullModeFlagBits::eNone,
			.frontFace = vk::FrontFace::eClockwise,
			.viewport = vk::Viewport {0, 0, (float)state.width, (float)state.height},
			.blendEnable = true,
			.depthTestEnable = true,
			.depthWriteEnable = true,
			.depthCompareOperation = vk::CompareOp::eLess,
		};
		pipeline.reset(new Pipeline(gpDesc, renderPass->vkRenderPass()));
		pipeline->allocateDescriptors({
			{.set = COLOR_SET, .count = 1},
			{.set = TEXTURES_AND_SAMPLER_SET, .count = 1},
			{.set = VERTEX_INDEX_SET, .count = 1},
			{.set = MATERIAL_SET, .count = 1},
			});
		pipeline->bindResource(COLOR_SET, 0, 0, uniformBuffer, 0, sizeof(glm::vec4), vk::DescriptorType::eUniformBuffer);
		pipeline->bindResource(TEXTURES_AND_SAMPLER_SET, 0, 0, { mesh.textures.begin(), mesh.textures.end() }, sampler);
		pipeline->bindResource(VERTEX_INDEX_SET, 0, 0, vertexBuffer, 0, vertexBuffer->size, vk::DescriptorType::eStorageBuffer);
		pipeline->bindResource(VERTEX_INDEX_SET, 1, 0, indiceBuffer, 0, indiceBuffer->size, vk::DescriptorType::eStorageBuffer);
		pipeline->bindResource(MATERIAL_SET, 0, 0, materialBuffer, 0, materialBuffer->size, vk::DescriptorType::eStorageBuffer);

	}
}

Application::~Application()
{
	auto device = Context::GetInstance().device;
	for (auto framebuffer : framebuffers)
	{
		device.destroyFramebuffer(framebuffer);
	}
	mesh.~Mesh();
	triangle.reset();
	vertexBuffer.reset();
	indiceBuffer.reset();
	materialBuffer.reset();
	uniformBuffer.reset();
	TextureManager::Instance().Destroy(texture);
	TextureManager::Instance().Destroy(depth);
	texture.reset();
	depth.reset();
	sampler.reset();
	renderPass.reset();
	pipeline.reset();
	VulkanBackend::Quit();
	DestroyWindow();
	EventManager::GetInstance().Unregister(EVENTCODE::APPLICATION_QUIT, nullptr, app_on_event);
	EventManager::GetInstance().Unregister(EVENTCODE::KEY_PRESSED, nullptr, app_on_key);
	EventManager::GetInstance().Unregister(EVENTCODE::KEY_RELEASED, nullptr, app_on_key);
	EventManager::GetInstance().Unregister(EVENTCODE::RESIZED, nullptr, app_on_resized);
	InputManager::Shutdown();
	EventManager::Shutdown();
}

void Application::run()
{
	std::vector<vk::Semaphore> imageAvaliables;
	std::vector<vk::Semaphore> imageDrawFinishs;
	std::vector<vk::Fence> cmdbufAvaliableFences;
	auto& device = Context::GetInstance().device;
	uint32_t maxFlight = Context::GetInstance().swapchain->info.imageCount;
	auto cmdbufs = CommandManager::Allocate(Context::GetInstance().graphicsCmdPool, maxFlight, true);
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


	vk::DescriptorPoolCreateInfo poolCI;
	vk::DescriptorPoolSize poolsize[2];
	poolsize[0].setDescriptorCount(maxFlight)
		.setType(vk::DescriptorType::eUniformBuffer);
	poolsize[1].setDescriptorCount(maxFlight * 2)
		.setType(vk::DescriptorType::eCombinedImageSampler);
	poolCI.setMaxSets(maxFlight + maxFlight)
		.setPoolSizes(poolsize);
	auto descriptorPool = Context::GetInstance().device.createDescriptorPool(poolCI);

	Buffer stage(sizeof(glm::vec4), vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
	void* ptr = device.mapMemory(stage.memory, 0, stage.size);

	{
		auto renderpass = renderPass->vkRenderPass();
		VulkanBackend::InitImGui(descriptorPool, renderpass);
	}
	ImGuiState imguistate;
	imguistate.addUI(GetTermination());
	imguistate.addUI(new ImGuiFrameTimeInfo(&state.timer));
	imguistate.addUI(&uniform);
	
	while (!WindowShouleClose())
	{
		state.timer.newFrame();
		auto deltatime = state.timer.lastFrameTime<std::chrono::milliseconds>();
		{
			ProcessInput(mainCamera, deltatime.count() / 1000.0);
			if (InputManager::GetInstance().GetIsKeyDown(KEY::KEY_Z))
			{
				rot += 3 * deltatime.count() / 5.0;
			}
			if (InputManager::GetInstance().GetIsKeyDown(KEY::KEY_X))
			{
				rot -= 3 * deltatime.count() / 5.0;
			}
		}
		glm::mat4 model = glm::rotate(glm::mat4(1.0), glm::radians(rot), glm::vec3(0, 0, -1));
		std::array<glm::mat4, 3> c;
		c[0] = model;
		c[1] = mainCamera.GetViewMatrix();
		c[2] = glm::perspective(glm::radians(45.0f), (float)state.width / state.height, 0.1f, 1000.0f);

		auto current_frame = Context::GetInstance().current_frame;
		memcpy(ptr, &uniform.color[0], stage.size);
		CopyBuffer(stage.buffer, uniformBuffer->buffer, stage.size, 0, 0);
		VulkanBackend::BeginFrame(deltatime.count() / 1000.0, cmdbufs[current_frame], cmdbufAvaliableFences[current_frame], imageAvaliables[current_frame]);
		imguistate.runUI();
		{
			vk::RenderPassBeginInfo renderPassBI;
			auto [width, height] = GetWindowSize();
			std::array<vk::ClearValue, 2> clear;
			clear[0].setColor({ 0.05f, 0.05f, 0.05f, 1.0f });
			clear[1].setDepthStencil(1.0f);
			renderPassBI.setRenderPass(renderPass->vkRenderPass())
				.setFramebuffer(framebuffers[Context::GetInstance().image_index])
				.setRenderArea(VkRect2D({ 0,0 }, { width, height }))
				.setClearValues(clear);
			cmdbufs[current_frame].beginRenderPass(renderPassBI, vk::SubpassContents::eInline);
			cmdbufs[current_frame].setViewport(0, { vk::Viewport{ 0, 0, (float)state.width, (float)state.height, 0.0f, 1.0f } });
			cmdbufs[current_frame].setScissor(0, { vk::Rect2D{vk::Offset2D{0, 0}, Context::GetInstance().swapchain->info.imageExtent} });
			cmdbufs[current_frame].setDepthTestEnable(VK_TRUE);
			pipeline->bind(cmdbufs[current_frame]);
			pipeline->bindDescriptorSets(cmdbufs[current_frame],
				{ {.set = COLOR_SET, .bindIdx = 0},
				{.set = TEXTURES_AND_SAMPLER_SET, .bindIdx = 0},
				{.set = VERTEX_INDEX_SET, .bindIdx = 0},
				{.set = MATERIAL_SET, .bindIdx = 0} });

			cmdbufs[current_frame].bindIndexBuffer(indiceBuffer->buffer, 0, vk::IndexType::eUint32);
			cmdbufs[current_frame].pushConstants(pipeline->vkPipelineLayout(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4) * 3, c.data());
			cmdbufs[current_frame].pushConstants(pipeline->vkPipelineLayout(), vk::ShaderStageFlagBits::eFragment, sizeof(glm::mat4) * 3, sizeof(glm::vec3), glm::value_ptr(mainCamera.Position));
			cmdbufs[current_frame].drawIndexed(mesh.indices.size(), 1, 0, 0, 0);

			// Rendering
			ImGui::Render();
			ImDrawData* draw_data = ImGui::GetDrawData();
			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdbufs[current_frame]);
		}
		cmdbufs[current_frame].endRenderPass();

		VulkanBackend::EndFrame(deltatime.count() / 1000.0, cmdbufs[current_frame], cmdbufAvaliableFences[current_frame], imageAvaliables[current_frame], imageDrawFinishs[current_frame]);
		WindowEventProcessing();
	}

	device.unmapMemory(stage.memory);
	device.waitIdle();
	VulkanBackend::CleanImGui();
	device.destroyDescriptorPool(descriptorPool);
	for (size_t i = 0; i < maxFlight; i++)
	{
		device.destroyFence(cmdbufAvaliableFences[i]);
		device.destroySemaphore(imageAvaliables[i]);
		device.destroySemaphore(imageDrawFinishs[i]);
	}
	CommandManager::Free(Context::GetInstance().graphicsCmdPool, cmdbufs);
}

/*-----------------------------------------------------------------*/
bool app_on_event(unsigned short code, void* sender, void* listener_inst, EventContext context)
{
	if (code == EVENTCODE::APPLICATION_QUIT)
	{
		return true;
	}
	return false;
}
bool app_on_key(unsigned short code, void* sender, void* listener_inst, EventContext context)
{
	if (code == EVENTCODE::KEY_PRESSED)
	{
		auto key_code = context.data.u16[0];
		if (key_code == KEY_ESCAPE)
		{
			EventContext data = {};
			EventManager::GetInstance().Fire(EVENTCODE::APPLICATION_QUIT, 0, data);
			return true;
		}
		else
		{
			//DEMO_LOG(Info, std::format("{} key pressed in window.", (char)key_code));
		}
	}
	else if(code == EVENTCODE::KEY_RELEASED)
	{
		auto key_code = context.data.u16[0];
		//DEMO_LOG(Info, std::format("{} key released in window.", (char)key_code));
	}
	return false;
}
bool app_on_resized(unsigned short code, void* sender, void* listener_inst, EventContext context)
{
	if (code == EVENTCODE::RESIZED)
	{
		auto width = context.data.u16[0];
		auto height = context.data.u16[1];
	}
	return false;
}