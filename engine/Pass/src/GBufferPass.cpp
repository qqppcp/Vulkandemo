#include "GBufferPass.h"
#include "render_process.h"
#include "Context.h"
#include "program.h"
#include "Texture.h"
#include "define.h"

constexpr uint32_t CAMERA_SET = 0;
constexpr uint32_t TEXTURES_SET = 1;
constexpr uint32_t SAMPLER_SET = 2;
constexpr uint32_t STORAGE_BUFFER_SET = 3;
// storing vertex/index/indirect/material buffer in array
constexpr uint32_t BINDING_0 = 0;
constexpr uint32_t BINDING_1 = 1;
constexpr uint32_t BINDING_2 = 2;
constexpr uint32_t BINDING_3 = 3;
constexpr uint32_t BINDING_4 = 4;

GBufferPass::GBufferPass()
{
}

GBufferPass::~GBufferPass()
{
	auto device = Context::GetInstance().device;
	device.destroyFramebuffer(m_framebuffer);
}

void GBufferPass::init(unsigned int width, unsigned int height)
{
	auto device = Context::GetInstance().device;
	initTextures(width, height);
	//{
	//	renderPass.reset(new RenderPass({ gBufferBaseColorTexture, gBufferNormalTexture,
	//		gBufferEmissiveTexture, gBufferSpecularTexture, gBufferPositionTexture,
	//		gBufferVelocityTexture, m_depthTexture }, {}, std::vector<vk::AttachmentLoadOp>{
	//		vk::AttachmentLoadOp::eClear, vk::AttachmentLoadOp::eClear, vk::AttachmentLoadOp::eClear,
	//			vk::AttachmentLoadOp::eClear, vk::AttachmentLoadOp::eClear, vk::AttachmentLoadOp::eClear
	//			, vk::AttachmentLoadOp::eClear}, std::vector<vk::AttachmentStoreOp>{
	//		vk::AttachmentStoreOp::eStore, vk::AttachmentStoreOp::eStore, vk::AttachmentStoreOp::eStore,
	//			vk::AttachmentStoreOp::eStore, vk::AttachmentStoreOp::eStore, vk::AttachmentStoreOp::eStore,
	//			vk::AttachmentStoreOp::eStore}, std::vector<vk::ImageLayout>{
	//			vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
	//				vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
	//				vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
	//				vk::ImageLayout::eShaderReadOnlyOptimal}, vk::PipelineBindPoint::eGraphics, "GBuffer RenderPass"));
	//}
	{
		renderPass.reset(new RenderPass(std::vector<vk::Format>{vk::Format::eR8G8B8A8Unorm, vk::Format::eR16G16B16A16Sfloat, vk::Format::eR16G16B16A16Sfloat, 
			vk::Format::eR8G8B8A8Unorm, vk::Format::eR16G16B16A16Sfloat, vk::Format::eR32G32Sfloat, vk::Format::eD24UnormS8Uint},
			std::vector<vk::ImageLayout>{vk::ImageLayout::eUndefined, vk::ImageLayout::eUndefined, vk::ImageLayout::eUndefined, vk::ImageLayout::eUndefined,
			vk::ImageLayout::eUndefined, vk::ImageLayout::eUndefined, vk::ImageLayout::eUndefined},
			std::vector<vk::ImageLayout>{vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
			vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, },
			std::vector<vk::AttachmentLoadOp>{vk::AttachmentLoadOp::eClear, vk::AttachmentLoadOp::eClear, vk::AttachmentLoadOp::eClear, vk::AttachmentLoadOp::eClear,
			vk::AttachmentLoadOp::eClear, vk::AttachmentLoadOp::eClear, vk::AttachmentLoadOp::eClear},
			std::vector<vk::AttachmentStoreOp>{vk::AttachmentStoreOp::eStore, vk::AttachmentStoreOp::eStore, vk::AttachmentStoreOp::eStore, vk::AttachmentStoreOp::eStore,
			vk::AttachmentStoreOp::eStore, vk::AttachmentStoreOp::eStore, vk::AttachmentStoreOp::eStore},
			vk::PipelineBindPoint::eGraphics, {}, 6));
	}
	{
		vk::FramebufferCreateInfo framebufferCI;
		std::array<vk::ImageView, 7> attachments{ gBufferBaseColorTexture->view,
		gBufferNormalTexture->view, gBufferEmissiveTexture->view,
		gBufferSpecularTexture->view, gBufferPositionTexture->view,
		gBufferVelocityTexture->view, m_depthTexture->view };
		framebufferCI.setAttachments(attachments)
			.setRenderPass(renderPass->vkRenderPass())
			.setWidth(width)
			.setHeight(height)
			.setLayers(1);
		m_framebuffer = device.createFramebuffer(framebufferCI);
	}
	{
		gBufferShader.reset(new GPUProgram(shaderPath + "gbuffer.vert.spv", shaderPath + "gbuffer.frag.spv"));
	}
	{
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

		std::vector<vk::PushConstantRange> ranges(1);
		ranges[0].setOffset(0)
			.setSize(sizeof(GBufferPushConstants))
			.setStageFlags(vk::ShaderStageFlagBits::eVertex);

		const Pipeline::GraphicsPipelineDescriptor gpDesc = {
			.sets = setLayouts,
			.vertexShader = gBufferShader->Vertex,
			.fragmentShader = gBufferShader->Fragment,
			.pushConstants = {ranges},
			.dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor},
			.colorTextureFormats = {vk::Format::eR8G8B8A8Unorm, vk::Format::eR16G16B16A16Sfloat,
				vk::Format::eR16G16B16A16Sfloat, vk::Format::eR8G8B8A8Unorm,
				vk::Format::eR16G16B16A16Sfloat, vk::Format::eR32G32Sfloat},
			.depthTextureFormat = vk::Format::eD24UnormS8Uint,
			.sampleCount = vk::SampleCountFlagBits::e1,
			.cullMode = vk::CullModeFlagBits::eNone,
			.frontFace = vk::FrontFace::eClockwise,
			.viewport = vk::Viewport {0, 0, 0, 0},
			.depthTestEnable = true,
			.depthWriteEnable = true,
			.depthCompareOperation = vk::CompareOp::eLess,
		};

		m_pipeline.reset(new Pipeline(gpDesc, renderPass->vkRenderPass()));
		m_pipeline->allocateDescriptors({
			{.set = CAMERA_SET, .count = 3},
			{.set = TEXTURES_SET, .count = 1},
			{.set = SAMPLER_SET, .count = 1},
			{.set = STORAGE_BUFFER_SET, .count = 1},
			});
	}
}

void GBufferPass::render(const std::vector<Pipeline::SetAndBindingIndex>& sets, 
	vk::Buffer indexBuffer, vk::Buffer indirectDrawBuffer, vk::Buffer indirectDrawCountBuffer, 
	uint32_t numMeshes, uint32_t bufferSize, bool applyJitter)
{
	auto current_frame = Context::GetInstance().current_frame;
	auto& cmdbufs = Context::GetInstance().cmdbufs;
	auto width = gBufferBaseColorTexture->width;
	auto height = gBufferBaseColorTexture->height;
	std::array<vk::ClearValue, 7> clearValues;
	clearValues[0].setColor({ 0.196f, 0.6f, 0.8f, 1.0f });
	clearValues[1].setColor({ 0.0f, 0.0f, 0.0f, 1.0f });
	clearValues[2].setColor({ 0.0f, 0.0f, 0.0f, 1.0f });
	clearValues[3].setColor({ 0.0f, 0.0f, 0.0f, 1.0f });
	clearValues[4].setColor({ 0.0f, 0.0f, 0.0f, 0.0f });
	clearValues[5].setColor({ 0.0f, 0.0f, 0.0f, 0.0f });
	clearValues[6].setDepthStencil(1.0f);

	vk::RenderPassBeginInfo renderPassBI;
	renderPassBI.setRenderPass(renderPass->vkRenderPass())
		.setFramebuffer(m_framebuffer)
		.setRenderArea(VkRect2D({ 0,0 }, { width, height }))
		.setClearValues(clearValues);

	cmdbufs[current_frame].beginRenderPass(renderPassBI, vk::SubpassContents::eInline);
	cmdbufs[current_frame].setViewport(0, { vk::Viewport{ 0, (float)height, (float)width, -(float)height, 0.0f, 1.0f } });
	cmdbufs[current_frame].setScissor(0, { vk::Rect2D{vk::Offset2D{0, 0}, vk::Extent2D{ width, height } } });
	GBufferPushConstants pushConst{
		.applyJitter = uint32_t(applyJitter),
	};
	m_pipeline->bind(cmdbufs[current_frame]);
	cmdbufs[current_frame].pushConstants(m_pipeline->vkPipelineLayout(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(GBufferPushConstants), &pushConst);
	m_pipeline->bindDescriptorSets(cmdbufs[current_frame], sets);
	m_pipeline->updateDescriptorSets();
	cmdbufs[current_frame].bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);
	cmdbufs[current_frame].drawIndexedIndirectCount(indirectDrawBuffer, 0, indirectDrawCountBuffer,
		0, numMeshes, bufferSize);
	cmdbufs[current_frame].endRenderPass();
	gBufferBaseColorTexture->layout = vk::ImageLayout::eShaderReadOnlyOptimal;
	gBufferNormalTexture->layout = vk::ImageLayout::eShaderReadOnlyOptimal;
	gBufferEmissiveTexture->layout = vk::ImageLayout::eShaderReadOnlyOptimal;
	gBufferSpecularTexture->layout = vk::ImageLayout::eShaderReadOnlyOptimal;
	gBufferPositionTexture->layout = vk::ImageLayout::eShaderReadOnlyOptimal;
	gBufferVelocityTexture->layout = vk::ImageLayout::eShaderReadOnlyOptimal;
	m_depthTexture->layout = vk::ImageLayout::eShaderReadOnlyOptimal;
}

void GBufferPass::initTextures(unsigned int width, unsigned int height)
{
	gBufferBaseColorTexture = TextureManager::Instance().Create(width, height, vk::Format::eR8G8B8A8Unorm,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
	vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst |
	vk::ImageUsageFlagBits::eTransferSrc);
	gBufferNormalTexture = TextureManager::Instance().Create(width, height, vk::Format::eR16G16B16A16Sfloat,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
		vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst |
		vk::ImageUsageFlagBits::eTransferSrc);
	gBufferEmissiveTexture = TextureManager::Instance().Create(width, height, vk::Format::eR16G16B16A16Sfloat,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
		vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst |
		vk::ImageUsageFlagBits::eTransferSrc);
	gBufferSpecularTexture = TextureManager::Instance().Create(width, height, vk::Format::eR8G8B8A8Unorm,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
		vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst |
		vk::ImageUsageFlagBits::eTransferSrc);
	gBufferVelocityTexture = TextureManager::Instance().Create(width, height, vk::Format::eR32G32Sfloat,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
		vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst |
		vk::ImageUsageFlagBits::eTransferSrc);
	gBufferPositionTexture = TextureManager::Instance().Create(width, height, vk::Format::eR16G16B16A16Sfloat,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
		vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst |
		vk::ImageUsageFlagBits::eTransferSrc);
	m_depthTexture = TextureManager::Instance().Create(width, height, vk::Format::eD24UnormS8Uint,
		vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferDst |
		vk::ImageUsageFlagBits::eSampled);
}
