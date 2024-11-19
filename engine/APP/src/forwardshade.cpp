#include "forwardshade.h"
#include <format>
#include <imgui_impl_vulkan.h>

#include "imguiLayer.h"
#include "SkyboxPass.h"
#include "CullingPass.h"
#include "ForwardPass.h"
#include "LightBoxPass.h"

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
	std::shared_ptr<ImGuiLayer> uiLayer;

	std::shared_ptr<SkyboxPass> skyboxPass;
	std::shared_ptr<ForwardPass> forwardPass;
	std::shared_ptr<CullingPass> cullingPass;
	std::shared_ptr<LightBoxPass> lightBoxPass;

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
	CameraManager::init({ 0.0f, 2.0f, 4.0f });
	GeometryManager::GetInstance().loadgltf(modelPath + "mirrors_edge_apartment_-_interior_scene.glb");
	uiLayer.reset(new ImGuiLayer());
	skyboxPass.reset(new SkyboxPass());
	forwardPass.reset(new ForwardPass());
	cullingPass.reset(new CullingPass());
	lightBoxPass.reset(new LightBoxPass());
	uiLayer->addUI(new CameraUI());
	uiLayer->addUI(forwardPass.get());
	std::vector<RenderTarget> rts(Context::GetInstance().swapchain->info.imageCount);
	for (int i = 0; i < rts.size(); i++)
	{
		rts[i].clears = { LoadOp::Clear, LoadOp::Clear };
		rts[i].formats = { Context::GetInstance().swapchain->info.surfaceFormat.format };
		rts[i].depthFormat = Context::GetInstance().depth->format;
		rts[i].views = { Context::GetInstance().swapchain->imageviews[i] };
		rts[i].depthView = Context::GetInstance().depth->view;
		rts[i].depth = 1;
		rts[i].width = Context::GetInstance().swapchain->info.imageExtent.width;
		rts[i].height = Context::GetInstance().swapchain->info.imageExtent.height;
	}
	skyboxPass->init(rts);
	for (int i = 0; i < rts.size(); i++)
	{
		rts[i].clears = { LoadOp::DontCare, LoadOp::DontCare };
	}
	forwardPass->init(rts);
	lightBoxPass->init(rts);
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

	samplers.emplace_back(new Sampler(vk::Filter::eNearest, vk::Filter::eNearest,
		vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
		vk::SamplerAddressMode::eRepeat, 10.0f));
	auto forwardPipeline = forwardPass->pipeline();
	forwardPipeline->bindResource(0, 0, 0, uniformBuffer, 0, sizeof(UniformTransforms), vk::DescriptorType::eUniformBuffer);
	forwardPipeline->bindResource(1, 0, 0, { glb->textures.begin(), glb->textures.end() });
	forwardPipeline->bindResource(2, 0, 0, { samplers.begin(), 1 });
	forwardPipeline->bindResource(3, 0, 0, { vertexBuffer, indiceBuffer, indirectBuffer, materialBuffer }, vk::DescriptorType::eStorageBuffer);
	cullingPass->init(glb, indirectBuffer);
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

	lightBoxPass.reset();
	cullingPass.reset();
	forwardPass.reset();
	skyboxPass.reset();
	uiLayer.reset();
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

	while (!WindowShouleClose())
	{
		UniformTransforms uniform;
		uniform.model = glm::mat4(1.0f);
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
		}
		VulkanBackend::BeginFrame(deltatime.count() / 1000.0, cmdbufs[current_frame], cmdbufAvaliableFences[current_frame], imageAvaliables[current_frame]);
		
		skyboxPass->render(cmdbufs[current_frame], Context::GetInstance().image_index);
		cullingPass->cull(cmdbufs[current_frame], Context::GetInstance().image_index);
		cullingPass->addBarrierForCulledBuffers(cmdbufs[current_frame], vk::PipelineStageFlagBits::eDrawIndirect,
			Context::GetInstance().queueFamileInfo.computeFamilyIndex.value(), Context::GetInstance().queueFamileInfo.graphicsFamilyIndex.value());
		forwardPass->render(cmdbufs[current_frame], Context::GetInstance().image_index,
			indiceBuffer->buffer, cullingPass->culledIndirectDrawBuffer()->buffer, cullingPass->culledIndirectDrawCountBuffer()->buffer,
			count, sizeof(IndirectCommandAndMeshData));
		lightBoxPass->render(cmdbufs[current_frame], Context::GetInstance().image_index,
			forwardPass->LightPos());

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
