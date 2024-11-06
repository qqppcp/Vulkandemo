#include "modelforwardLayer.h"
#include "mesh.h"
#include "Sampler.h"
#include "render_process.h"
#include "Pipeline.h"
#include "geometry.h"
#include "program.h"
#include "Buffer.h"
#include "define.h"
#include "Context.h"
#include "window.h"
#include "camera.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace
{
	std::vector<vk::Framebuffer> framebuffers;
	void* ptr = nullptr;
	float rot = 180;
}

ModelForwardLayer::ModelForwardLayer(std::string_view path)
{
	color = { 1.0f, 1.0f, 1.0f, 1.0f };
	blinn_phong.reset(new GPUProgram(shaderPath + "blinn-phong.vert.spv", shaderPath + "blinn-phong.frag.spv"));
	mesh = GeometryManager::GetInstance().getMesh(path.data());
	//mesh->loadgltf(modelPath + "Bistro.glb");
	//mesh->loadobj(path.data());
	vertexBuffer.reset(new Buffer(mesh->vertices.size() * sizeof(Vertex), vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal));
	indiceBuffer.reset(new Buffer(mesh->indices.size() * sizeof(std::uint32_t), vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndexBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal));
	materialBuffer.reset(new Buffer(mesh->materials.size() * sizeof(Material), vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal));
	uniformBuffer.reset(new Buffer(sizeof(glm::vec4), vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal));
	UploadBufferData({}, vertexBuffer, mesh->vertices.size() * sizeof(Vertex), mesh->vertices.data());
	UploadBufferData({}, indiceBuffer, mesh->indices.size() * sizeof(std::uint32_t), mesh->indices.data());
	UploadBufferData({}, materialBuffer, mesh->materials.size() * sizeof(Material), mesh->materials.data());
	UploadBufferData({}, uniformBuffer, sizeof(glm::vec4), &color[0]);
	{
		sampler.reset(new Sampler(vk::Filter::eLinear, vk::Filter::eLinear,
			vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
			vk::SamplerAddressMode::eRepeat, 10.0f));
	}
	{
		renderPass.reset(new RenderPass(std::vector<vk::Format>{Context::GetInstance().swapchain->info.surfaceFormat.format, vk::Format::eD24UnormS8Uint},
			std::vector<vk::ImageLayout>{vk::ImageLayout::eUndefined, vk::ImageLayout::eUndefined},
			std::vector<vk::ImageLayout>{vk::ImageLayout::ePresentSrcKHR, vk::ImageLayout::eDepthStencilAttachmentOptimal },
			std::vector<vk::AttachmentLoadOp>{vk::AttachmentLoadOp::eClear, vk::AttachmentLoadOp::eClear},
			std::vector<vk::AttachmentStoreOp>{vk::AttachmentStoreOp::eStore, vk::AttachmentStoreOp::eStore},
			vk::PipelineBindPoint::eGraphics, {}, 1));
	}
	{
		for (int i = 0; i < Context::GetInstance().swapchain->imageviews.size(); i++)
		{
			auto [width, height] = GetWindowSize();
			std::vector<vk::ImageView> attachments{ Context::GetInstance().swapchain->imageviews[i], Context::GetInstance().depth->view };
			vk::FramebufferCreateInfo framebufferCI;
			framebufferCI.setAttachments(attachments)
				.setLayers(1)
				.setRenderPass(renderPass->vkRenderPass())
				.setWidth(width)
				.setHeight(height);
			framebuffers.emplace_back(Context::GetInstance().device.createFramebuffer(framebufferCI));
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
			.vertexShader = blinn_phong->Vertex,
			.fragmentShader = blinn_phong->Fragment,
			.pushConstants = {ranges},
			.dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor,
				vk::DynamicState::eDepthTestEnable},
			.colorTextureFormats = {Context::GetInstance().swapchain->info.surfaceFormat.format},
			.depthTextureFormat = vk::Format::eD24UnormS8Uint,
			.cullMode = vk::CullModeFlagBits::eNone,
			.frontFace = vk::FrontFace::eClockwise,
			.viewport = vk::Viewport {0, 0, 0, 0},
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
		pipeline->bindResource(TEXTURES_AND_SAMPLER_SET, 0, 0, { mesh->textures.begin(), mesh->textures.end() }, sampler);
		pipeline->bindResource(VERTEX_INDEX_SET, 0, 0, vertexBuffer, 0, vertexBuffer->size, vk::DescriptorType::eStorageBuffer);
		pipeline->bindResource(VERTEX_INDEX_SET, 1, 0, indiceBuffer, 0, indiceBuffer->size, vk::DescriptorType::eStorageBuffer);
		pipeline->bindResource(MATERIAL_SET, 0, 0, materialBuffer, 0, materialBuffer->size, vk::DescriptorType::eStorageBuffer);
	}
	stageBuffer.reset(new Buffer(sizeof(glm::vec4), vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));
	ptr = Context::GetInstance().device.mapMemory(stageBuffer->memory, 0, stageBuffer->size);
}

ModelForwardLayer::~ModelForwardLayer()
{
	Context::GetInstance().device.unmapMemory(stageBuffer->memory);
	for (auto framebuffer : framebuffers)
	{
		Context::GetInstance().device.destroyFramebuffer(framebuffer);
	}
	mesh.reset();
	blinn_phong.reset();
	stageBuffer.reset();
	vertexBuffer.reset();
	indiceBuffer.reset();
	materialBuffer.reset();
	uniformBuffer.reset();
	sampler.reset();
	renderPass.reset();
	pipeline.reset();
}

void ModelForwardLayer::OnUpdate(float _deltatime)
{
	if (InputManager::GetInstance().GetIsKeyDown(KEY::KEY_Z))
	{
		rot += 600 * _deltatime;
	}
	if (InputManager::GetInstance().GetIsKeyDown(KEY::KEY_X))
	{
		rot -= 600 * _deltatime;
	}
}

void ModelForwardLayer::OnRender()
{
	memcpy(ptr, glm::value_ptr(color), sizeof(glm::vec4));
	CopyBuffer(stageBuffer->buffer, uniformBuffer->buffer, sizeof(glm::vec4), 0, 0);
	auto current_frame = Context::GetInstance().current_frame;
	auto& cmdbufs = Context::GetInstance().cmdbufs;
	auto [width, height] = GetWindowSize();
	glm::mat4 model = glm::rotate(glm::mat4(1.0), glm::radians(rot), glm::vec3(0, 0, -1));
	std::array<glm::mat4, 3> c;
	c[0] = model;
	c[1] = CameraManager::mainCamera->GetViewMatrix();
	c[2] = glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 1000.0f);
	vk::RenderPassBeginInfo renderPassBI;

	std::array<vk::ClearValue, 2> clear;
	clear[0].setColor({ 0.05f, 0.05f, 0.05f, 1.0f });
	clear[1].setDepthStencil(1.0f);
	renderPassBI.setRenderPass(renderPass->vkRenderPass())
		.setFramebuffer(framebuffers[Context::GetInstance().image_index])
		.setRenderArea(VkRect2D({ 0,0 }, { width, height }))
		.setClearValues(clear);
	cmdbufs[current_frame].beginRenderPass(renderPassBI, vk::SubpassContents::eInline);
	cmdbufs[current_frame].setViewport(0, { vk::Viewport{ 0, 0, (float)width, (float)height, 0.0f, 1.0f } });
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
	cmdbufs[current_frame].pushConstants(pipeline->vkPipelineLayout(), vk::ShaderStageFlagBits::eFragment, sizeof(glm::mat4) * 3, sizeof(glm::vec3), glm::value_ptr(CameraManager::mainCamera->Position));
	cmdbufs[current_frame].drawIndexed(mesh->indices.size(), 1, 0, 0, 0);
	cmdbufs[current_frame].endRenderPass();
}

void ModelForwardLayer::customUI()
{
	if (!isOpen)
	{
		return;
	}

	if (ImGui::CollapsingHeader("ModelForwardLayer", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::ColorEdit3("Light Color", glm::value_ptr(color));
	}
	
};