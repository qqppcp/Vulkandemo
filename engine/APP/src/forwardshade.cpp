#include "forwardshade.h"
#include <format>
#include <imgui_impl_vulkan.h>

#include "imguiLayer.h"
#include "ClearPass.h"
#include "SkyboxPass.h"
#include "CullingPass.h"
#include "ForwardPass.h"
#include "VelocityPass.h"
#include "TAAPass.h"
#include "LightBoxPass.h"
#include "FullScreenPass.h"

#include "FrameTimeInfo.h"
#include "termination.h"
#include "geometry.h"
#include "Pipeline.h"
#include "backend.h"
#include "Context.h"
#include "Sampler.h"
#include "Texture.h"
#include "Buffer.h"
#include "camera.h"
#include "define.h"
#include "window.h"
#include "mesh.h"

namespace
{
	std::shared_ptr<Texture> colorTexture;
	std::shared_ptr<Texture> depthTexture;

	std::shared_ptr<ImGuiLayer> uiLayer;

	std::shared_ptr<ClearPass> clearPass;
	std::shared_ptr<SkyboxPass> skyboxPass;
	std::shared_ptr<ForwardPass> forwardPass;
	std::shared_ptr<VelocityPass> velocityPass;
	std::shared_ptr<CullingPass> cullingPass;
	std::shared_ptr<TAAPass> taaPass;
	std::shared_ptr<LightBoxPass> lightBoxPass;
	std::shared_ptr<FullScreenPass> fullScreenPass;

	std::shared_ptr<Buffer> stageBuffer;
	std::shared_ptr<Buffer> vertexBuffer;
	std::shared_ptr<Buffer> indiceBuffer;
	std::shared_ptr<Buffer> materialBuffer;
	std::shared_ptr<Buffer> indirectBuffer;
	std::shared_ptr<Buffer> indirectCountBuffer;
	std::shared_ptr<Buffer> uniformBuffer;
	std::shared_ptr<Buffer> lightBuffer;
	std::vector < std::shared_ptr < Sampler >> samplers;
	void* ptr = nullptr;
	int count;

}

void ForwardShade::Init(uint32_t width, uint32_t height)
{
	state.width = width, state.height = height;
	CameraManager::init({ 0.0f, 2.0f, 4.0f });
	GeometryManager::GetInstance().loadgltf(modelPath + "mirrors_edge_apartment_-_interior_scene.glb");
	colorTexture = TextureManager::Instance().Create(width, height, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eColorAttachment |
		vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst |
		vk::ImageUsageFlagBits::eTransferSrc);
	depthTexture = TextureManager::Instance().Create(width, height, vk::Format::eD24UnormS8Uint, vk::ImageUsageFlagBits::eDepthStencilAttachment |
		vk::ImageUsageFlagBits::eSampled );
	uiLayer.reset(new ImGuiLayer());
	clearPass.reset(new ClearPass);
	skyboxPass.reset(new SkyboxPass());
	forwardPass.reset(new ForwardPass());
	velocityPass.reset(new VelocityPass());
	cullingPass.reset(new CullingPass());
	taaPass.reset(new TAAPass());
	lightBoxPass.reset(new LightBoxPass());
	fullScreenPass.reset(new FullScreenPass(false));
	uiLayer->addUI(new ImGuiFrameTimeInfo(&state.timer));
	uiLayer->addUI(new CameraUI());
	uiLayer->addUI(forwardPass.get());
	clearPass->init(colorTexture, depthTexture);
	skyboxPass->init(colorTexture, depthTexture);
	forwardPass->init(colorTexture, depthTexture);
	velocityPass->init(width, height);
	taaPass->init(depthTexture, velocityPass->velocityTexture(), colorTexture);
	lightBoxPass->init(colorTexture, depthTexture);
	auto glb = GeometryManager::GetInstance().getMesh(modelPath + "mirrors_edge_apartment_-_interior_scene.glb");
	vertexBuffer.reset(new Buffer(glb->vertices.size() * sizeof(Vertex), vk::BufferUsageFlagBits::eShaderDeviceAddress | 
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal));
	indiceBuffer.reset(new Buffer(glb->indices.size() * sizeof(std::uint32_t), vk::BufferUsageFlagBits::eShaderDeviceAddress | 
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndexBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal));
	materialBuffer.reset(new Buffer(glb->materials.size() * sizeof(Material), vk::BufferUsageFlagBits::eShaderDeviceAddress | 
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal));
	uniformBuffer.reset(new Buffer(sizeof(UniformTransforms), vk::BufferUsageFlagBits::eShaderDeviceAddress | 
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal));
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
	auto forwardPipeline = forwardPass->pipeline();
	forwardPipeline->bindResource(0, 0, 0, uniformBuffer, 0, sizeof(UniformTransforms), vk::DescriptorType::eUniformBuffer);
	forwardPipeline->bindResource(1, 0, 0, { glb->textures.begin(), glb->textures.end() });
	forwardPipeline->bindResource(2, 0, 0, { samplers.begin(), 1 });
	forwardPipeline->bindResource(3, 0, 0, { vertexBuffer, indiceBuffer, indirectBuffer, materialBuffer }, vk::DescriptorType::eStorageBuffer);
	auto velocityPipeline = velocityPass->pipeline();
	velocityPipeline->bindResource(0, 0, 0, uniformBuffer, 0, sizeof(UniformTransforms), vk::DescriptorType::eUniformBuffer);
	velocityPipeline->bindResource(3, 0, 0, { vertexBuffer, indiceBuffer, indirectBuffer, materialBuffer }, vk::DescriptorType::eStorageBuffer);
	cullingPass->init(glb, indirectBuffer);
	fullScreenPass->init({ Context::GetInstance().swapchain->info.surfaceFormat.format });
	fullScreenPass->pipeline()->bindResource(0, 0, 0, { taaPass->ColorTexture()
		}, samplers.back());
}

void ForwardShade::Shutdown()
{
	auto device = Context::GetInstance().device;
	device.unmapMemory(stageBuffer->memory);
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
	indirectBuffer.reset();

	fullScreenPass.reset();
	lightBoxPass.reset();
	cullingPass.reset();
	velocityPass.reset();
	taaPass.reset();
	forwardPass.reset();
	skyboxPass.reset();
	clearPass.reset();
	uiLayer.reset();
	colorTexture.reset();
	depthTexture.reset();
}

void ForwardShade::run()
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
	UniformTransforms uniform;
	int frameIndex = 0;
	
	while (!WindowShouleClose())
	{
		uniform.model = glm::mat4(1.0f);
		bool isCamMoving = !(uniform.prevViewMat == CameraManager::mainCamera->GetViewMatrix());
		uniform.prevViewMat = uniform.view;
		uniform.view = CameraManager::mainCamera->GetViewMatrix();
		uniform.projection = glm::perspective(glm::radians(45.0f), (float)1280 / 720, 0.1f, 1000.0f);
		uniform.jitter = CameraManager::JitterMat(frameIndex, 16, state.width, state.height);
		memcpy(ptr, &uniform, sizeof(UniformTransforms));
		CopyBuffer(stageBuffer->buffer, uniformBuffer->buffer, sizeof(UniformTransforms), 0, 0);

		state.timer.newFrame();
		auto deltatime = state.timer.lastFrameTime<std::chrono::milliseconds>();
		auto current_frame = Context::GetInstance().current_frame;
		auto& cmdbufs = Context::GetInstance().cmdbufs;
		{
			ProcessInput(*CameraManager::mainCamera, deltatime.count() / 1000.0);
		}
		VulkanBackend::BeginFrame(deltatime.count() / 1000.0, cmdbufs[current_frame], cmdbufAvaliableFences[current_frame], imageAvaliables[current_frame]);
		clearPass->render(cmdbufs[current_frame], 0);
		skyboxPass->render(cmdbufs[current_frame], 0);
		cullingPass->cull(cmdbufs[current_frame], 0);
		cullingPass->addBarrierForCulledBuffers(cmdbufs[current_frame], vk::PipelineStageFlagBits::eDrawIndirect,
			Context::GetInstance().queueFamileInfo.computeFamilyIndex.value(), Context::GetInstance().queueFamileInfo.graphicsFamilyIndex.value());
		forwardPass->render(cmdbufs[current_frame], 0,
			indiceBuffer->buffer, cullingPass->culledIndirectDrawBuffer()->buffer, cullingPass->culledIndirectDrawCountBuffer()->buffer,
			count, sizeof(IndirectCommandAndMeshData), true);
		velocityPass->render(cmdbufs[current_frame], 0,
			indiceBuffer->buffer, cullingPass->culledIndirectDrawBuffer()->buffer, cullingPass->culledIndirectDrawCountBuffer()->buffer,
			count, sizeof(IndirectCommandAndMeshData));
		lightBoxPass->render(cmdbufs[current_frame], 0,
			forwardPass->LightPos());
		taaPass->doAA(cmdbufs[current_frame], frameIndex, isCamMoving);

		fullScreenPass->render(Context::GetInstance().image_index, false);
		frameIndex++;
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
}
