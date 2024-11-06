#include "FullScreenPass.h"
#include "Context.h"
#include "program.h"
#include "define.h"
#include "Pipeline.h"
#include "render_process.h"
#include <glm/glm.hpp>

struct FullScreenPushConst {
	glm::vec4 showAsDepth;
};

FullScreenPass::FullScreenPass(bool useDynamicRendering)
	:bUseDynamicRendering(useDynamicRendering){}

FullScreenPass::~FullScreenPass()
{
	for (auto framebuffer : m_framebuffers)
	{
		Context::GetInstance().device.destroyFramebuffer(framebuffer);
	}
}

void FullScreenPass::init(std::vector<vk::Format> colorTextureFormats)
{
	width = Context::GetInstance().swapchain->info.imageExtent.width;
	height = Context::GetInstance().swapchain->info.imageExtent.height;
	if (!bUseDynamicRendering)
	{
		m_renderPass.reset(new RenderPass(std::vector<vk::Format>{Context::GetInstance().swapchain->info.surfaceFormat.format},
			std::vector<vk::ImageLayout>{vk::ImageLayout::eUndefined},
			std::vector<vk::ImageLayout>{vk::ImageLayout::ePresentSrcKHR},
			std::vector<vk::AttachmentLoadOp>{vk::AttachmentLoadOp::eClear},
			std::vector<vk::AttachmentStoreOp>{vk::AttachmentStoreOp::eStore},
			vk::PipelineBindPoint::eGraphics, {}, 1));
		for (int i = 0; i < Context::GetInstance().swapchain->imageviews.size(); i++)
		{
			std::vector<vk::ImageView> attachments{ Context::GetInstance().swapchain->imageviews[i] };
			vk::FramebufferCreateInfo framebufferCI;
			framebufferCI.setAttachments(attachments)
				.setLayers(1)
				.setRenderPass(m_renderPass->vkRenderPass())
				.setWidth(width)
				.setHeight(height);
			m_framebuffers.emplace_back(Context::GetInstance().device.createFramebuffer(framebufferCI));
		}
	}
	fullScreenShader.reset(new GPUProgram(shaderPath + "fullscreen.vert.spv", shaderPath + "fullscreen.frag.spv"));
	std::vector<Pipeline::SetDescriptor> setLayouts;
	{
		Pipeline::SetDescriptor set;
		set.set = 0;
		vk::DescriptorSetLayoutBinding binding;
		binding.setBinding(0)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setStageFlags(vk::ShaderStageFlagBits::eFragment);
		set.bindings.push_back(binding);
		setLayouts.push_back(set);
	}
	std::vector<vk::PushConstantRange> ranges(1);
	ranges[0].setOffset(0)
		.setSize(sizeof(FullScreenPushConst))
		.setStageFlags(vk::ShaderStageFlagBits::eFragment);
	const Pipeline::GraphicsPipelineDescriptor gpDesc = {
		.sets = setLayouts,
		.vertexShader = fullScreenShader->Vertex,
		.fragmentShader = fullScreenShader->Fragment,
		.pushConstants = {ranges},
		.dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor},
		.useDynamicRendering = bUseDynamicRendering,
		.colorTextureFormats = colorTextureFormats,
		.primitiveTopology = vk::PrimitiveTopology::eTriangleStrip,
		.sampleCount = vk::SampleCountFlagBits::e1,
		.cullMode = vk::CullModeFlagBits::eNone,
		.frontFace = vk::FrontFace::eClockwise,
		.viewport = {0, 0, (float)width, (float)height},
		.depthTestEnable = false,
		.depthWriteEnable = false,
	};
	m_pipeline.reset(new Pipeline(gpDesc, m_renderPass->vkRenderPass()));
	m_pipeline->allocateDescriptors({
		{.set = 0, .count = 1}
		});
}

void FullScreenPass::render(uint32_t index, bool showAsDepth)
{
	auto current_frame = Context::GetInstance().current_frame;
	auto& cmdbufs = Context::GetInstance().cmdbufs;
	std::array<vk::ClearValue, 1> clear;
	clear[0].setColor({ 0.05f, 0.05f, 0.05f, 1.0f });
	if (!bUseDynamicRendering)
	{
		vk::RenderPassBeginInfo renderPassBI;
		renderPassBI.setRenderPass(m_renderPass->vkRenderPass())
			.setFramebuffer(m_framebuffers[index])
			.setRenderArea(VkRect2D({ 0,0 }, { width, height }))
			.setClearValues(clear);
		cmdbufs[current_frame].beginRenderPass(renderPassBI, vk::SubpassContents::eInline);
	}
	else
	{
		// TODO
	}
	cmdbufs[current_frame].setViewport(0, { vk::Viewport{ 0, (float)height, (float)width, -(float)height, 0.0f, 1.0f} });
	cmdbufs[current_frame].setScissor(0, { vk::Rect2D{vk::Offset2D{0, 0}, Context::GetInstance().swapchain->info.imageExtent} });
	FullScreenPushConst pushConst;
	pushConst.showAsDepth.x = showAsDepth;
	m_pipeline->updatePushConstant(cmdbufs[current_frame], vk::ShaderStageFlagBits::eFragment, sizeof(FullScreenPushConst), &pushConst);
	m_pipeline->bind(cmdbufs[current_frame]);
	m_pipeline->bindDescriptorSets(cmdbufs[current_frame],
		{ {.set = 0, .bindIdx = 0}, });
	m_pipeline->updateDescriptorSets();
	cmdbufs[current_frame].draw(4, 1, 0, 0);
	if (!bUseDynamicRendering)
	{
		cmdbufs[current_frame].endRenderPass();
	}
	else
	{
		//TODO
	}
}
