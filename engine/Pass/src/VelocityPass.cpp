#include "VelocityPass.h"
#include "Context.h"
#include "Texture.h"
#include "Pipeline.h"
#include "program.h"
#include "define.h"
#include "render_process.h"

constexpr uint32_t CAMERA_SET = 0U;
constexpr uint32_t TEXTURES_SET = 1U;
constexpr uint32_t SAMPLER_SET = 2U;
constexpr uint32_t STORAGE_BUFFER_SET = 3U;

VelocityPass::~VelocityPass()
{
	Context::GetInstance().device.destroyFramebuffer(framebuffer);
}

void VelocityPass::init(uint32_t width, uint32_t height)
{
	this->width = width;
	this->height = height;
	outVelocityTexture = TextureManager::Instance().Create(width, height, vk::Format::eR32G32Sfloat,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
		vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst |
		vk::ImageUsageFlagBits::eTransferSrc);
	
	renderPass.reset(new RenderPass(std::vector<vk::Format>{vk::Format::eR32G32Sfloat},
		std::vector<vk::ImageLayout>{vk::ImageLayout::eUndefined},
		std::vector<vk::ImageLayout>{vk::ImageLayout::eShaderReadOnlyOptimal},
		std::vector<vk::AttachmentLoadOp>{vk::AttachmentLoadOp::eClear},
		std::vector<vk::AttachmentStoreOp>{vk::AttachmentStoreOp::eStore},
		vk::PipelineBindPoint::eGraphics, {}, 1));

	std::vector<vk::ImageView> attachments{ outVelocityTexture->view };
	vk::FramebufferCreateInfo frameInfo;
	frameInfo.setAttachments(attachments)
		.setHeight(height)
		.setWidth(width)
		.setRenderPass(renderPass->vkRenderPass())
		.setLayers(1);
	framebuffer = Context::GetInstance().device.createFramebuffer(frameInfo);

	auto shader = std::make_shared<GPUProgram>(shaderPath + "velocity.vert.spv", shaderPath + "velocity.frag.spv");
	std::vector<Pipeline::SetDescriptor> setLayouts;
	{
		Pipeline::SetDescriptor set;
		set.set = CAMERA_SET;
		vk::DescriptorSetLayoutBinding binding;
		binding.setBinding(0)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
		set.bindings.push_back(binding);
		setLayouts.push_back(set);
	}
	{
		Pipeline::SetDescriptor set;
		set.set = TEXTURES_SET;
		vk::DescriptorSetLayoutBinding binding;
		binding.setBinding(0)
			.setDescriptorCount(500)
			.setDescriptorType(vk::DescriptorType::eSampledImage)
			.setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
		set.bindings.push_back(binding);
		setLayouts.push_back(set);
	}
	{
		Pipeline::SetDescriptor set;
		set.set = SAMPLER_SET;
		vk::DescriptorSetLayoutBinding binding;
		binding.setBinding(0)
			.setDescriptorCount(50)
			.setDescriptorType(vk::DescriptorType::eSampler)
			.setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
		set.bindings.push_back(binding);
		setLayouts.push_back(set);
	}
	{
		Pipeline::SetDescriptor set;
		set.set = STORAGE_BUFFER_SET;
		vk::DescriptorSetLayoutBinding binding;
		binding.setBinding(0)
			.setDescriptorCount(4)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
		set.bindings.push_back(binding);
		setLayouts.push_back(set);
	}
	const Pipeline::GraphicsPipelineDescriptor gpDesc = {
			.sets = setLayouts,
			.vertexShader = shader->Vertex,
			.fragmentShader = shader->Fragment,
			.dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor},
			.colorTextureFormats = {outVelocityTexture->format},
			.cullMode = vk::CullModeFlagBits::eNone,
			.frontFace = vk::FrontFace::eClockwise,
			.viewport = vk::Viewport {0, 0, 0, 0},
			.depthTestEnable = true,
			.depthWriteEnable = true,
			.depthCompareOperation = vk::CompareOp::eLess,
	};
	pipeline_.reset(new Pipeline(gpDesc, renderPass->vkRenderPass()));
	pipeline_->allocateDescriptors({
			{.set = CAMERA_SET, .count = 1},
			{.set = TEXTURES_SET, .count = 1},
			{.set = SAMPLER_SET, .count = 1},
			{.set = STORAGE_BUFFER_SET, .count = 1},
		});
}

void VelocityPass::render(vk::CommandBuffer cmdbuf, uint32_t index, vk::Buffer indexBuffer,
	vk::Buffer indirectDrawBuffer, vk::Buffer indirectDrawCountBuffer,
	uint32_t numMeshes, uint32_t bufferSize)
{
	vk::RenderPassBeginInfo renderPassBI;

	std::array<vk::ClearValue, 1> clear;
	clear[0].setColor({ 0.0f, 0.0f, 0.0f, 0.0f });
	renderPassBI.setRenderPass(renderPass->vkRenderPass())
		.setFramebuffer(framebuffer)
		.setRenderArea(VkRect2D({ 0,0 }, { width, height }))
		.setClearValues(clear);
	cmdbuf.beginRenderPass(renderPassBI, vk::SubpassContents::eInline);
	{
		cmdbuf.setViewport(0, { vk::Viewport{ 0, (float)height, (float)width, -(float)height, 0.0f, 1.0f} });
		cmdbuf.setScissor(0, { vk::Rect2D{vk::Offset2D{0, 0}, vk::Extent2D{ width, height } } });
		pipeline_->bind(cmdbuf);
		pipeline_->bindDescriptorSets(cmdbuf, {
			{.set = CAMERA_SET, .bindIdx = 0},
			{.set = STORAGE_BUFFER_SET, .bindIdx = 0},
			});
		pipeline_->updateDescriptorSets();
		cmdbuf.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);
		cmdbuf.drawIndexedIndirectCount(indirectDrawBuffer, 0, indirectDrawCountBuffer,
			0, numMeshes, bufferSize);
	}
	cmdbuf.endRenderPass();
	outVelocityTexture->layout = vk::ImageLayout::eShaderReadOnlyOptimal;
}
