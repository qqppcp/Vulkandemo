#include "defershade.h"
#include <format>
#include <imgui_impl_vulkan.h>

#include "imguiLayer.h"
#include "GBufferPass.h"
#include "FullScreenPass.h"
#include "ShadowMapPass.h"
#include "CullingPass.h"
#include "HierarchicalDepthBufferPass.h"
#include "SSAOPass.h"
#include "NoisePass.h"
#include "LightingPass.h"
#include "SSRIntersectPass.h"
#include "LineBoxPass.h"

#include "FrameTimeInfo.h"
#include "termination.h"
#include "geometry.h"
#include "backend.h"
#include "Context.h"
#include "camera.h"
#include "define.h"
#include "window.h"
#include "mesh.h"

namespace
{
	std::shared_ptr<ImGuiLayer> uiLayer;

	std::shared_ptr<Buffer> stageBuffer;
	std::shared_ptr<Buffer> vertexBuffer;
	std::shared_ptr<Buffer> indiceBuffer;
	std::shared_ptr<Buffer> materialBuffer;
	std::shared_ptr<Buffer> indirectBuffer;
	std::shared_ptr<Buffer> indirectCountBuffer;
	std::shared_ptr<Buffer> uniformBuffer;
	std::shared_ptr<Buffer> lightBuffer;

	std::shared_ptr<GBufferPass> gbufferPass;
	std::shared_ptr<FullScreenPass> fullScreenPass;
	std::shared_ptr<ShadowMapPass> shadowPass;
	std::shared_ptr<CullingPass> cullingPass;
	std::shared_ptr<HierarchicalDepthBufferPass> hierarchicalDepthBufferPass;
	std::shared_ptr<SSAOPass> ssaoPass;
	std::shared_ptr<NoisePass> noisePass;
	std::shared_ptr<LightingPass> lightPass;
	std::shared_ptr<SSRIntersectPass> ssrPass;
	std::shared_ptr<LineBoxPass> lineBoxPass;
	std::vector < std::shared_ptr < Sampler >> samplers;
	void* ptr = nullptr;
	int count;
}

void DeferShade::Init(uint32_t width, uint32_t height)
{
	CameraManager::init({ 0.0f, 2.0f, 4.0f });
	GeometryManager::GetInstance().loadgltf(modelPath + "mirrors_edge_apartment_-_interior_scene.glb");
	uiLayer.reset(new ImGuiLayer());
	gbufferPass.reset(new GBufferPass());
	gbufferPass->init(width, height);
	fullScreenPass.reset(new FullScreenPass(false));
	fullScreenPass->init({ Context::GetInstance().swapchain->info.surfaceFormat.format });
	cullingPass.reset(new CullingPass());
	shadowPass.reset(new ShadowMapPass());
	hierarchicalDepthBufferPass.reset(new HierarchicalDepthBufferPass());
	ssaoPass.reset(new SSAOPass());
	noisePass.reset(new NoisePass());
	lightPass.reset(new LightingPass());
	ssrPass.reset(new SSRIntersectPass());
	lineBoxPass.reset(new LineBoxPass());
	uiLayer->addUI(GetTermination());
	uiLayer->addUI(new ImGuiFrameTimeInfo(&state.timer));
	uiLayer->addUI(new CameraUI());
	uiLayer->addUI(gbufferPass.get());
	auto gbufferPipeline = gbufferPass->pipeline();
	auto glb = GeometryManager::GetInstance().getMesh(modelPath + "mirrors_edge_apartment_-_interior_scene.glb");
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

	samplers.emplace_back(new Sampler(vk::Filter::eNearest, vk::Filter::eNearest,
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
	shadowPipeline->bindResource(1, 0, 0, { glb->textures.begin(), glb->textures.begin() + 1 });
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
	lineBoxPass->init(lightPass->lightTexture(), gbufferPass->depthTexture(), glb);
	fullScreenPass->pipeline()->bindResource(0, 0, 0, { ssrPass->intersectTexture()
		}, samplers.back());
}

void DeferShade::Shutdown()
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

	lineBoxPass.reset();
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
}

void DeferShade::run()
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
	gbufferPass->setImageId(descriptorSet);

	LightData lightData;
	{
		glm::vec3 lightPos = glm::vec3(0.1, 3.0, 2.0);
		glm::vec3 target = glm::vec3(0.f);
		glm::vec3 front = glm::normalize(target - lightPos);
		glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));
		glm::vec3 up = glm::normalize(glm::cross(right, front));
		UniformTransforms uniform;
		uniform.model = glm::mat4(1.0f);
		uniform.view = glm::lookAt(lightPos, target, up);
		uniform.projection = glm::perspective(glm::radians(60.0f), (float)1280 / 1280, 0.1f, 15.0f);
		memcpy(ptr, &uniform, sizeof(UniformTransforms));
		CopyBuffer(stageBuffer->buffer, lightBuffer->buffer, sizeof(UniformTransforms), 0, 0);
		lightData.ambientColor = glm::vec4(1.f, 1.f, 1.f, 1.f);
		lightData.lightColor = glm::vec4(1.f, 1.f, 1.f, 1.f);
		lightData.lightVP = uniform.projection * uniform.view;
		lightData.lightDir = glm::vec4(front, 1.f);
		lightData.lightPos = glm::vec4(lightPos, 1.0f);
	}

	UniformTransforms uniform;
	while (!WindowShouleClose())
	{
		uniform.model = glm::rotate<float>(glm::mat4(1.0), glm::radians<float>(0), glm::vec3(0, 0, -1));
		uniform.model = glm::mat4(1.0f);
		uniform.prevViewMat = uniform.view;
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
		//lineBoxPass->render(cmdbufs[current_frame], uniform.view, uniform.projection);
		ssrPass->run(cmdbufs[current_frame]);
		fullScreenPass->render(Context::GetInstance().image_index, false);
		//ImTextureID id = (ImTextureID)descriptorSet;
		//uiLayer->setimageid(id);
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
