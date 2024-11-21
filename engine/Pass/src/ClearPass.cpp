#include "ClearPass.h"
#include "Context.h"
#include "Texture.h"
#include "program.h"
#include "Pipeline.h"
#include "render_process.h"


ClearPass::~ClearPass()
{
	Context::GetInstance().device.destroyFramebuffer(framebuffer);
}

void ClearPass::init(std::shared_ptr<Texture> color, std::shared_ptr<Texture> depth)
{
	colorTexture_ = color;
	depthTexture_ = depth;
	width = color->width;
	height = color->height;
	renderPass.reset(new RenderPass({color->format, depth->format},
		std::vector<vk::ImageLayout>{vk::ImageLayout::eUndefined, vk::ImageLayout::eUndefined},
		std::vector<vk::ImageLayout>{vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eDepthStencilAttachmentOptimal },
		std::vector<vk::AttachmentLoadOp> {vk::AttachmentLoadOp::eClear, vk::AttachmentLoadOp::eClear},
		std::vector<vk::AttachmentStoreOp>{vk::AttachmentStoreOp::eStore, vk::AttachmentStoreOp::eStore},
		vk::PipelineBindPoint::eGraphics, {}, 1));
	std::vector<vk::ImageView> attachments{ color->view, depth->view };
	vk::FramebufferCreateInfo framebufferCI;
	framebufferCI.setAttachments(attachments)
		.setLayers(1)
		.setRenderPass(renderPass->vkRenderPass())
		.setWidth(width)
		.setHeight(height);
	framebuffer = Context::GetInstance().device.createFramebuffer(framebufferCI);
}

void ClearPass::render(vk::CommandBuffer cmdbuf, uint32_t index)
{
	std::vector<vk::ClearValue> clears(2);
	clears[0].setColor({ 0.5f, 0.3f, 0.6f, 1.0f });
	clears[1].setDepthStencil(1.0f);

	vk::RenderPassBeginInfo renderPassBI;
	renderPassBI.setRenderPass(renderPass->vkRenderPass())
		.setClearValues(clears)
		.setFramebuffer(framebuffer)
		.setRenderArea(VkRect2D({ 0,0 }, { width, height }));
	cmdbuf.beginRenderPass(renderPassBI, vk::SubpassContents::eInline);
	cmdbuf.endRenderPass();
}

