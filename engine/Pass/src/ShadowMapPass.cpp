#include "ShadowMapPass.h"

#include "render_process.h"
#include "program.h"
#include "Texture.h"
#include "Context.h"
#include "define.h"

constexpr uint32_t CAMERA_SET = 0;
constexpr uint32_t TEXTURES_SET = 1;
constexpr uint32_t SAMPLER_SET = 2;
constexpr uint32_t STORAGE_BUFFER_SET = 3;

namespace {
	constexpr int ShadowMapSize = 4096;
}

ShadowMapPass::ShadowMapPass()
{
}

ShadowMapPass::~ShadowMapPass()
{
	for (auto framebuffer : m_framebuffers)
	{
		Context::GetInstance().device.destroyFramebuffer(framebuffer);
	}
	m_renderPass.reset();
	m_pipeline.reset();
	m_shadowmap.reset();
	m_shadowShader.reset();
}

void ShadowMapPass::init()
{
	vk::Format depthFormat = vk::Format::eD24UnormS8Uint;
	{
		m_shadowmap = TextureManager::Instance().Create(ShadowMapSize, ShadowMapSize, depthFormat, vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled);
	}
	{
		m_renderPass.reset(new RenderPass(std::vector<vk::Format>{depthFormat},
			std::vector<vk::ImageLayout>{vk::ImageLayout::eUndefined},
			std::vector<vk::ImageLayout>{vk::ImageLayout::eShaderReadOnlyOptimal },
			std::vector<vk::AttachmentLoadOp>{vk::AttachmentLoadOp::eClear},
			std::vector<vk::AttachmentStoreOp>{vk::AttachmentStoreOp::eStore},
			vk::PipelineBindPoint::eGraphics, {}, 0));
	}
	{
		vk::FramebufferCreateInfo createInfo;
		createInfo.setAttachments(m_shadowmap->view)
			.setLayers(1)
			.setRenderPass(m_renderPass->vkRenderPass())
			.setWidth(ShadowMapSize)
			.setHeight(ShadowMapSize);
		m_framebuffers.push_back(Context::GetInstance().device.createFramebuffer(createInfo));
	}
	{
		m_shadowShader.reset(new GPUProgram(shaderPath + "shadowmap.vert.spv", shaderPath + "void.frag.spv"));
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
				.setStageFlags(vk::ShaderStageFlagBits::eVertex);
			set.bindings.push_back(binding);
			setLayouts.push_back(set);
		}
		{
			Pipeline::SetDescriptor set;
			set.set = TEXTURES_SET;
			vk::DescriptorSetLayoutBinding binding;
			binding.setBinding(0)
				.setDescriptorCount(1)
				.setDescriptorType(vk::DescriptorType::eSampledImage)
				.setStageFlags(vk::ShaderStageFlagBits::eVertex);
			set.bindings.push_back(binding);
			setLayouts.push_back(set);
		}
		{
			Pipeline::SetDescriptor set;
			set.set = SAMPLER_SET;
			vk::DescriptorSetLayoutBinding binding;
			binding.setBinding(0)
				.setDescriptorCount(1)
				.setDescriptorType(vk::DescriptorType::eSampler)
				.setStageFlags(vk::ShaderStageFlagBits::eVertex);
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
			.vertexShader = m_shadowShader->Vertex,
			.fragmentShader = m_shadowShader->Fragment,
			.dynamicStates = {vk::DynamicState::eViewport , vk::DynamicState::eScissor},
			.colorTextureFormats = {},
			.depthTextureFormat = depthFormat,
			.sampleCount = vk::SampleCountFlagBits::e1,
			.cullMode = vk::CullModeFlagBits::eBack,
			.viewport = {0 ,0 ,0 ,0 },
			.depthTestEnable = true,
			.depthWriteEnable = true,
			.depthCompareOperation = vk::CompareOp::eLess,
		};
		m_pipeline.reset(new Pipeline(gpDesc, m_renderPass->vkRenderPass()));
		m_pipeline->allocateDescriptors({
			{.set = CAMERA_SET, .count = 1},
			{.set = TEXTURES_SET, .count = 1},
			{.set = SAMPLER_SET, .count = 1},
			{.set = STORAGE_BUFFER_SET, .count = 1},
			});
	}
}

void ShadowMapPass::render(const std::vector<Pipeline::SetAndBindingIndex>& sets, 
	vk::Buffer indexBuffer, vk::Buffer indirectDrawBuffer, 
	uint32_t numMeshes, uint32_t bufferSize)
{
	auto current_frame = Context::GetInstance().current_frame;
	auto& cmdbufs = Context::GetInstance().cmdbufs;
	std::array<vk::ClearValue, 1> clear;
	clear[0].setDepthStencil(1.0f);
	vk::RenderPassBeginInfo renderPassBI;
	renderPassBI.setClearValues(clear)
		.setFramebuffer(m_framebuffers[0])
		.setRenderArea(VkRect2D({ 0,0 }, { ShadowMapSize, ShadowMapSize }))
		.setRenderPass(m_renderPass->vkRenderPass());
	cmdbufs[current_frame].beginRenderPass(renderPassBI, vk::SubpassContents::eInline);
	cmdbufs[current_frame].setViewport(0, { vk::Viewport{ 0, (float)ShadowMapSize, (float)ShadowMapSize, -(float)ShadowMapSize, 0.0f, 1.0f } });
	cmdbufs[current_frame].setScissor(0, { vk::Rect2D{vk::Offset2D{0, 0}, vk::Extent2D{ ShadowMapSize, ShadowMapSize } } });
	m_pipeline->bind(cmdbufs[current_frame]);
	m_pipeline->bindDescriptorSets(cmdbufs[current_frame], sets);
	m_pipeline->updateDescriptorSets();
	cmdbufs[current_frame].bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);
	cmdbufs[current_frame].drawIndexedIndirect(indirectDrawBuffer, 0, numMeshes, bufferSize);
	cmdbufs[current_frame].endRenderPass();
	m_shadowmap->layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
}

