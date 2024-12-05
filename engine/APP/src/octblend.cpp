#include "octblend.h"
#include <format>
#include <imgui_impl_vulkan.h>

#include "imguiLayer.h"
#include "ForwardPass.h"
#include "FullScreenPass.h"

#include "FrameTimeInfo.h"
#include "termination.h"
#include "geometry.h"
#include "render_process.h"
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

	std::vector<std::shared_ptr<Texture>> colors;
	std::vector<vk::Framebuffer> framebuffers;
	
	std::shared_ptr<ForwardPass> forwardPass;
	std::shared_ptr<FullScreenPass> fullScreenPass;

	std::vector < std::shared_ptr < Sampler >> samplers;
	std::shared_ptr<Buffer> stageBuffer;
	std::shared_ptr<Buffer> vertexBuffer;
	std::shared_ptr<Buffer> indiceBuffer;
	std::shared_ptr<Buffer> materialBuffer;
	std::shared_ptr<Buffer> indirectBuffer;
	std::shared_ptr<Buffer> indirectCountBuffer;
	std::shared_ptr<Buffer> uniformBuffer;
	void* ptr = nullptr;
	int count;

	glm::mat4 cubeViews[8];
	std::shared_ptr<Mesh> mesh;
}

void OctBlend::Init(uint32_t width, uint32_t height)
{
	mesh.reset(new Mesh());
	std::vector<glm::vec3> positions = {
		glm::vec3(0, 0, 0),
		glm::vec3(1, 0, 0),
		glm::vec3(1, 1, 0),
		glm::vec3(1, 1, 0),
		glm::vec3(0, 1, 0),
		glm::vec3(0, 0, 0)
	};
	std::vector<glm::vec2> uvs = {
		glm::vec2(0, 0),
		glm::vec2(1, 0),
		glm::vec2(1, 1),
		glm::vec2(1, 1),
		glm::vec2(0, 1),
		glm::vec2(0, 0)
	};
	mesh->vertices.resize(6);
	for (int i = 0; i < 6; i++)
	{
		mesh->vertices[i].Position = positions[i];
		mesh->vertices[i].TexCoords = uvs[i];
		mesh->vertices[i].Normal = glm::vec3(0, 0, 1);
		mesh->indices.push_back(i);
	}
	IndirectCommandAndMeshData indirect;
	indirect.command.setFirstIndex(0)
		.setFirstInstance(0)
		.setIndexCount(6)
		.setInstanceCount(1)
		.setVertexOffset(0);
	indirect.materialIndex = 0;
	indirect.meshId = 0;
	mesh->indirectDrawData.push_back(indirect);
	auto texture = TextureManager::Instance().Load(texturePath + "matrix.jpg");
	mesh->textures.push_back(texture);
	Material mat;
	mat.diffuseTextureId = 0;
	mesh->materials.push_back(mat);
	cubeViews[0] = glm::lookAt(glm::vec3(0, 0, 1), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	cubeViews[1] = glm::lookAt(glm::vec3(1, 0, 1), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	cubeViews[2] = glm::lookAt(glm::vec3(1, 1, 1), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	cubeViews[3] = glm::lookAt(glm::vec3(0, 1, 1), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	cubeViews[4] = glm::lookAt(glm::vec3(0, 0, 2), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	cubeViews[5] = glm::lookAt(glm::vec3(1, 0, 2), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	cubeViews[6] = glm::lookAt(glm::vec3(1, 1, 2), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	cubeViews[7] = glm::lookAt(glm::vec3(0, 1, 2), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

	vertexBuffer.reset(new Buffer(mesh->vertices.size() * sizeof(Vertex), vk::BufferUsageFlagBits::eShaderDeviceAddress |
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal));
	indiceBuffer.reset(new Buffer(mesh->indices.size() * sizeof(std::uint32_t), vk::BufferUsageFlagBits::eShaderDeviceAddress |
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndexBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal));
	materialBuffer.reset(new Buffer(mesh->materials.size() * sizeof(Material), vk::BufferUsageFlagBits::eShaderDeviceAddress |
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal));
	uniformBuffer.reset(new Buffer(sizeof(UniformTransforms), vk::BufferUsageFlagBits::eShaderDeviceAddress |
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal));
	stageBuffer.reset(new Buffer(sizeof(UniformTransforms), vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible));
	ptr = Context::GetInstance().device.mapMemory(stageBuffer->memory, 0, stageBuffer->size);
	indirectBuffer.reset(new Buffer(mesh->indirectDrawData.size() * sizeof(IndirectCommandAndMeshData), vk::BufferUsageFlagBits::eShaderDeviceAddress |
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal));
	indirectCountBuffer.reset(new Buffer(sizeof(int), vk::BufferUsageFlagBits::eShaderDeviceAddress |
		vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal));
	UploadBufferData({}, vertexBuffer, mesh->vertices.size() * sizeof(Vertex), mesh->vertices.data());
	UploadBufferData({}, indiceBuffer, mesh->indices.size() * sizeof(std::uint32_t), mesh->indices.data());
	UploadBufferData({}, materialBuffer, mesh->materials.size() * sizeof(Material), mesh->materials.data());
	UploadBufferData({}, indirectBuffer, mesh->indirectDrawData.size() * sizeof(IndirectCommandAndMeshData), mesh->indirectDrawData.data());
	count = mesh->indirectDrawData.size();
	UploadBufferData({}, indirectCountBuffer, sizeof(int), &count);

	CameraManager::init({ 0.0f, 0.0f, 4.0f });
	colorTexture = TextureManager::Instance().Create(width, height, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eColorAttachment |
		vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst |
		vk::ImageUsageFlagBits::eTransferSrc);
	depthTexture = TextureManager::Instance().Create(width, height, vk::Format::eD24UnormS8Uint, vk::ImageUsageFlagBits::eDepthStencilAttachment |
		vk::ImageUsageFlagBits::eSampled);

	samplers.emplace_back(new Sampler(vk::Filter::eLinear, vk::Filter::eLinear,
		vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
		vk::SamplerAddressMode::eRepeat, 10.0f));

	forwardPass.reset(new ForwardPass());
	forwardPass->init(colorTexture, depthTexture, true);

	fullScreenPass.reset(new FullScreenPass(false));
	auto forwardPipeline = forwardPass->pipeline();
	forwardPipeline->bindResource(0, 0, 0, uniformBuffer, 0, sizeof(UniformTransforms), vk::DescriptorType::eUniformBuffer);
	forwardPipeline->bindResource(1, 0, 0, { mesh->textures.begin(), mesh->textures.end() });
	forwardPipeline->bindResource(2, 0, 0, { samplers.begin(), 1 });
	forwardPipeline->bindResource(3, 0, 0, { vertexBuffer, indiceBuffer, indirectBuffer, materialBuffer }, vk::DescriptorType::eStorageBuffer);
	fullScreenPass->init({ Context::GetInstance().swapchain->info.surfaceFormat.format });
	fullScreenPass->pipeline()->bindResource(0, 0, 0, { colorTexture }, samplers.back());

	colors.resize(8);
	framebuffers.resize(8);
	for (int i = 0; i < 8; i++)
	{
		colors[i] = TextureManager::Instance().Create(width, height, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eColorAttachment |
			vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst |
			vk::ImageUsageFlagBits::eTransferSrc);
		std::vector<vk::ImageView> attachments{ colors[i]->view, depthTexture->view};
		vk::FramebufferCreateInfo framebufferCI;
		framebufferCI.setAttachments(attachments)
			.setLayers(1)
			.setRenderPass(forwardPass->renderPass()->vkRenderPass())
			.setWidth(width)
			.setHeight(height);
		framebuffers[i] = Context::GetInstance().device.createFramebuffer(framebufferCI);
	}
}

void OctBlend::Shutdown()
{
	auto device = Context::GetInstance().device;
	device.unmapMemory(stageBuffer->memory);
	for (auto sampler : samplers)
	{
		sampler.reset();
	}
	samplers.clear();
	for (auto color : colors)
	{
		color.reset();
	}
	colors.clear();
	for (auto framebuffer : framebuffers)
	{
		device.destroyFramebuffer(framebuffer);
	}
	framebuffers.clear();

	stageBuffer.reset();
	vertexBuffer.reset();
	indiceBuffer.reset();
	materialBuffer.reset();
	uniformBuffer.reset();
	indirectBuffer.reset();
	indirectCountBuffer.reset();
	indirectBuffer.reset();

	fullScreenPass.reset();
	forwardPass.reset();
	mesh.reset();
	colorTexture.reset();
	depthTexture.reset();
}

void OctBlend::run()
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
		uniform.view = CameraManager::mainCamera->GetViewMatrix();
		//uniform.view = glm::lookAt(glm::vec3(0, 0, 4), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
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
		for (int i = 0; i < 8; i++)
		{
			uniform.view = cubeViews[i];
			memcpy(ptr, &uniform, sizeof(UniformTransforms));
			CopyBuffer(stageBuffer->buffer, uniformBuffer->buffer, sizeof(UniformTransforms), 0, 0);
		}
		uniform.view = CameraManager::mainCamera->GetViewMatrix();
		memcpy(ptr, &uniform, sizeof(UniformTransforms));
		CopyBuffer(stageBuffer->buffer, uniformBuffer->buffer, sizeof(UniformTransforms), 0, 0);
		forwardPass->render(cmdbufs[current_frame], 0,
			indiceBuffer->buffer, indirectBuffer->buffer, indirectCountBuffer->buffer,
			count, sizeof(IndirectCommandAndMeshData));

		fullScreenPass->render(Context::GetInstance().image_index, false);
		frameIndex++;
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
