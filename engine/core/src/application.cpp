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
#include "ImGuiState.h"
#include "termination.h"
#include "FrameTimeInfo.h"
#include "CommandBuffer.h"
#include "GBufferPass.h"
#include "FullScreenPass.h"
#include "ShadowMapPass.h"
#include "CullingPass.h"
#include "HierarchicalDepthBufferPass.h"
#include "SSAOPass.h"
#include "NoisePass.h"
#include "LightingPass.h"
#include "SSRIntersectPass.h"
#include "geometry.h"
#include "camera.h"
#include "Texture.h"
#include "Sampler.h"
#include "Pipeline.h"
#include "modelforwardLayer.h"
#include "skyboxLayer.h"
#include "imguiLayer.h"
#include <glm/gtc/matrix_transform.hpp>

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
	std::string shaderPath = R"(assets\shaders\)";
	std::string texturePath = R"(assets\textures\)";
	std::string modelPath = R"(assets\models\)";
	std::shared_ptr<ModelForwardLayer> layer;
	std::shared_ptr<SkyboxLayer> skyboxLayer;
	std::shared_ptr<ImGuiLayer> uiLayer;
	std::shared_ptr<GBufferPass> gbufferPass;
	std::shared_ptr<FullScreenPass> fullScreenPass;
	std::shared_ptr<ShadowMapPass> shadowPass;
	std::shared_ptr<Buffer> stageBuffer;
	std::shared_ptr<Buffer> vertexBuffer;
	std::shared_ptr<Buffer> indiceBuffer;
	std::shared_ptr<Buffer> materialBuffer;
	std::shared_ptr<Buffer> indirectBuffer;
	std::shared_ptr<Buffer> indirectCountBuffer;
	std::shared_ptr<Buffer> uniformBuffer;
	std::shared_ptr<Buffer> lightBuffer;
	std::shared_ptr<CullingPass> cullingPass;
	std::shared_ptr<HierarchicalDepthBufferPass> hierarchicalDepthBufferPass;
	std::shared_ptr<SSAOPass> ssaoPass;
	std::shared_ptr<NoisePass> noisePass;
	std::shared_ptr<LightingPass> lightPass;
	std::shared_ptr<SSRIntersectPass> ssrPass;
	std::vector < std::shared_ptr < Sampler >> samplers;
	void* ptr = nullptr;
	int count;
	float rot = 180;
}


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
	CameraManager::init({ -10, 2, 1 });
	CreateWindow(width, height, name.data());
	VulkanBackend::Init();
	GeometryManager::Init();
	GeometryManager::GetInstance().loadobj(modelPath + "nanosuit_reflect/nanosuit.obj");
	GeometryManager::GetInstance().loadgltf(modelPath + "Bistro.glb");
	layer.reset(new ModelForwardLayer(modelPath + "nanosuit_reflect/nanosuit.obj"));
	skyboxLayer.reset(new SkyboxLayer(texturePath + "skybox.hdr"));
	uiLayer.reset(new ImGuiLayer());
	gbufferPass.reset(new GBufferPass());
	gbufferPass->init(width, height);
	fullScreenPass.reset(new FullScreenPass(false));
	fullScreenPass->init({ vk::Format::eR8G8B8A8Unorm });
	cullingPass.reset(new CullingPass());
	shadowPass.reset(new ShadowMapPass());
	hierarchicalDepthBufferPass.reset(new HierarchicalDepthBufferPass());
	ssaoPass.reset(new SSAOPass());
	noisePass.reset(new NoisePass());
	lightPass.reset(new LightingPass());
	ssrPass.reset(new SSRIntersectPass());
	uiLayer->addUI(GetTermination());
	uiLayer->addUI(new ImGuiFrameTimeInfo(&state.timer));
	uiLayer->addUI(layer.get());
	auto gbufferPipeline = gbufferPass->pipeline();
	auto glb = GeometryManager::GetInstance().getMesh(modelPath + "Bistro.glb");
	vertexBuffer.reset(new Buffer(glb->vertices.size() * sizeof(Vertex), vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal));
	indiceBuffer.reset(new Buffer(glb->indices.size() * sizeof(std::uint32_t), vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndexBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal));
	materialBuffer.reset(new Buffer(glb->materials.size() * sizeof(Material), vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal));
	uniformBuffer.reset(new Buffer(sizeof(UniformTransforms), vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal));
	lightBuffer.reset(new Buffer(sizeof(UniformTransforms), vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst |
		vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal));
	stageBuffer.reset(new Buffer(sizeof(UniformTransforms), vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible));
	ptr = Context::GetInstance().device.mapMemory(stageBuffer->memory, 0, stageBuffer->size);
	indirectBuffer.reset(new Buffer(glb->indirectDrawData.size() * sizeof(IndirectCommandAndMeshData), vk::BufferUsageFlagBits::eShaderDeviceAddress |
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal));
	indirectCountBuffer.reset(new Buffer(sizeof(int), vk::BufferUsageFlagBits::eShaderDeviceAddress |
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal));
	UploadBufferData({}, vertexBuffer, glb->vertices.size() * sizeof(Vertex), glb->vertices.data());
	UploadBufferData({}, indiceBuffer, glb->indices.size() * sizeof(std::uint32_t), glb->indices.data());
	UploadBufferData({}, materialBuffer, glb->materials.size() * sizeof(Material), glb->materials.data());
	UploadBufferData({}, indirectBuffer, glb->indirectDrawData.size() * sizeof(IndirectCommandAndMeshData), glb->indirectDrawData.data());
	count = glb->indirectDrawData.size();
	UploadBufferData({}, indirectCountBuffer, sizeof(int), &count);

	samplers.emplace_back(new Sampler(vk::Filter::eLinear, vk::Filter::eLinear,
		vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
		vk::SamplerAddressMode::eRepeat, 10.0f));
	gbufferPipeline->bindResource(0, 0, 0, uniformBuffer, 0, sizeof(UniformTransforms), vk::DescriptorType::eUniformBuffer);
	gbufferPipeline->bindResource(1, 0, 0, { glb->textures.begin(), glb->textures.end() });
	gbufferPipeline->bindResource(2, 0, 0, { samplers.begin(), 1 });
	gbufferPipeline->bindResource(3, 0, 0, { vertexBuffer, indiceBuffer, indirectBuffer, materialBuffer }, vk::DescriptorType::eStorageBuffer);
	cullingPass->init(glb, indirectBuffer);
	shadowPass->init();
	auto shadowPipeline = shadowPass->pipeline();
	shadowPipeline->bindResource(0, 0, 0, lightBuffer, 0, sizeof(UniformTransforms), vk::DescriptorType::eUniformBuffer);
	shadowPipeline->bindResource(1, 0, 0, {glb->textures.begin(), glb->textures.begin() + 1});
	shadowPipeline->bindResource(2, 0, 0, { samplers.begin(), 1 });
	shadowPipeline->bindResource(3, 0, 0, { vertexBuffer, indiceBuffer, indirectBuffer, materialBuffer }, vk::DescriptorType::eStorageBuffer);
	hierarchicalDepthBufferPass->init(gbufferPass->depthTexture());
	ssaoPass->init(gbufferPass->depthTexture());
	noisePass->init();
	lightPass->init(gbufferPass->normalTexture(), gbufferPass->specularTexture(),
		gbufferPass->baseColorTexture(), gbufferPass->positionTexture(),
		gbufferPass->depthTexture(), ssaoPass->ssaoTexture(), shadowPass->shadowmap());
	ssrPass->init(gbufferPass->normalTexture(), gbufferPass->specularTexture(),
		lightPass->lightTexture(), hierarchicalDepthBufferPass->hierarchicalDepthTexture(),
		noisePass->noiseTexture());

	fullScreenPass->pipeline()->bindResource(0, 0, 0, { ssrPass->intersectTexture()
		}, samplers.back());
}

Application::~Application()
{
	Context::GetInstance().device.unmapMemory(stageBuffer->memory);
	for (auto sampler : samplers)
	{
		sampler.reset();
	}
	samplers.clear();
	stageBuffer.reset();
	vertexBuffer.reset();
	indiceBuffer.reset();
	materialBuffer.reset();
	uniformBuffer.reset();
	lightBuffer.reset();
	indirectBuffer.reset();
	indirectCountBuffer.reset();
	auto device = Context::GetInstance().device;
	indirectBuffer.reset();
	ssrPass.reset();
	lightPass.reset();
	noisePass.reset();
	ssaoPass.reset();
	hierarchicalDepthBufferPass.reset();
	shadowPass.reset();
	cullingPass.reset();
	fullScreenPass.reset();
	gbufferPass.reset();
	uiLayer.reset();
	skyboxLayer.reset();
	layer.reset();
	TextureManager::Instance().Clear();
	GeometryManager::Quit();
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

	vk::DescriptorSetLayoutBinding layoutBinding;
	layoutBinding.setBinding(0)
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
		.setDescriptorCount(1)
		.setStageFlags(vk::ShaderStageFlagBits::eFragment);
	vk::DescriptorSetLayoutCreateInfo layoutInfo;
	layoutInfo.setBindings(layoutBinding);
	auto setlayout = device.createDescriptorSetLayout(layoutInfo);
	vk::DescriptorSetAllocateInfo allocInfo;
	allocInfo.setDescriptorPool(uiLayer->descriptorPool)
		.setDescriptorSetCount(1)
		.setSetLayouts(setlayout);
	vk::DescriptorSet descriptorSet = device.allocateDescriptorSets(allocInfo)[0];
	vk::DescriptorImageInfo imageInfo;
	imageInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
		.setImageView(gbufferPass->baseColorTexture()->view)
		.setSampler(samplers[0]->vkSampler());
	vk::WriteDescriptorSet descriptorWrite;
	descriptorWrite.setDstSet(descriptorSet)
		.setDstBinding(0)
		.setDstArrayElement(0)
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
		.setDescriptorCount(1)
		.setImageInfo(imageInfo);
	device.updateDescriptorSets(descriptorWrite, {});
	LightData lightData;
	{
		Camera lightCamera(glm::vec3(-3.5, 30, 2.0), glm::vec3(0.0f, 1.0f, 0.0f), 90.0f, -90.0f);
		UniformTransforms uniform;
		uniform.model = glm::mat4(1.0f);
		uniform.view = lightCamera.GetViewMatrix();
		//uniform.view = CameraManager::mainCamera->GetViewMatrix();
		//uniform.projection = glm::ortho(-500.0f, 500.0f, -500.0f, 500.0f, -500.0f, 1000.0f);
		uniform.projection = glm::perspective(glm::radians(60.0f), (float)1280 / 1280, 0.1f, 200.0f);
		memcpy(ptr, &uniform, sizeof(UniformTransforms));
		CopyBuffer(stageBuffer->buffer, lightBuffer->buffer, sizeof(UniformTransforms), 0, 0);
		lightData.lightCam = lightCamera;
		lightData.ambientColor = glm::vec4(1.f, 1.f, 1.f, 1.f);
		lightData.lightColor = glm::vec4(1.f, 1.f, 1.f, 1.f);
		lightData.lightVP = uniform.projection * uniform.view;
		lightData.lightDir = glm::vec4(lightCamera.Front, 1.f);
		lightData.lightPos = glm::vec4(10, 85, 0, 1);
	}
	

	while (!WindowShouleClose())
	{
		UniformTransforms uniform;
		uniform.model = glm::rotate<float>(glm::mat4(1.0), glm::radians<float>(0), glm::vec3(0, 0, -1));
		uniform.view = CameraManager::mainCamera->GetViewMatrix();
		uniform.projection = glm::perspective(glm::radians(45.0f), (float)1280 / 720, 0.1f, 1000.0f);
		memcpy(ptr, &uniform, sizeof(UniformTransforms));
		CopyBuffer(stageBuffer->buffer, uniformBuffer->buffer, sizeof(UniformTransforms), 0, 0);
		state.timer.newFrame();
		auto deltatime = state.timer.lastFrameTime<std::chrono::milliseconds>();
		auto current_frame = Context::GetInstance().current_frame;
		auto& cmdbufs = Context::GetInstance().cmdbufs;
		{
			ProcessInput(*CameraManager::mainCamera, deltatime.count() / 1000.0);
			layer->OnUpdate(deltatime.count() / 1000.0);
		}
		VulkanBackend::BeginFrame(deltatime.count() / 1000.0, cmdbufs[current_frame], cmdbufAvaliableFences[current_frame], imageAvaliables[current_frame]);
		cullingPass->cull(cmdbufs[current_frame], Context::GetInstance().image_index);
		cullingPass->addBarrierForCulledBuffers(cmdbufs[current_frame], vk::PipelineStageFlagBits::eDrawIndirect,
			Context::GetInstance().queueFamileInfo.computeFamilyIndex.value(), Context::GetInstance().queueFamileInfo.graphicsFamilyIndex.value());

		gbufferPass->render({
			{.set = 0, .bindIdx = 0},
			{.set = 1, .bindIdx = 0},
			{.set = 2, .bindIdx = 0},
			{.set = 3, .bindIdx = 0}
			}, indiceBuffer->buffer, cullingPass->culledIndirectDrawBuffer()->buffer, cullingPass->culledIndirectDrawCountBuffer()->buffer, count, sizeof(IndirectCommandAndMeshData));
		shadowPass->render({
			{.set = 0, .bindIdx = 0},
			{.set = 1, .bindIdx = 0},
			{.set = 2, .bindIdx = 0},
			{.set = 3, .bindIdx = 0}
			}, indiceBuffer->buffer, indirectBuffer->buffer, count, sizeof(IndirectCommandAndMeshData));
		//layer->OnRender();
		//skyboxLayer->OnRender();
		noisePass->generateNoise(cmdbufs[current_frame]);
		hierarchicalDepthBufferPass->generateHierarchicalDepthBuffer(cmdbufs[current_frame]);
		ssaoPass->run(cmdbufs[current_frame]);
		lightPass->render(cmdbufs[current_frame], Context::GetInstance().image_index, lightData,
			uniform.view, uniform.projection);
		ssrPass->run(cmdbufs[current_frame]);
		fullScreenPass->render(Context::GetInstance().image_index, false);
		ImTextureID id = (ImTextureID)descriptorSet;
		uiLayer->setimageid(id);
		uiLayer->OnRender();

		VulkanBackend::EndFrame(deltatime.count() / 1000.0, cmdbufs[current_frame], cmdbufAvaliableFences[current_frame], imageAvaliables[current_frame], imageDrawFinishs[current_frame]);
		WindowEventProcessing();
	}

	device.waitIdle();
	for (size_t i = 0; i < maxFlight; i++)
	{
		device.destroyFence(cmdbufAvaliableFences[i]);
		device.destroySemaphore(imageAvaliables[i]);
		device.destroySemaphore(imageDrawFinishs[i]);
	}
	device.destroyDescriptorSetLayout(setlayout);
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
	else if (code == EVENTCODE::KEY_RELEASED)
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