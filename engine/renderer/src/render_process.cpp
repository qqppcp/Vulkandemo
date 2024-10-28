#include "render_process.h"
#include "Context.h"
#include "program.h"
#include "window.h"
#include "Vertex.h"
#include "Texture.h"

#include <optional>


RenderPass::RenderPass(const std::vector<std::shared_ptr<Texture>> attachments, const std::vector<std::shared_ptr<Texture>> resolveAttachments, 
	const std::vector<vk::AttachmentLoadOp>& loadOp, const std::vector<vk::AttachmentStoreOp>& storeOp, 
	const std::vector<vk::ImageLayout>& layout, vk::PipelineBindPoint bindPoint, const std::string& name)
{
	std::vector<vk::AttachmentDescription> attachmentDescriptors;
	std::vector<vk::AttachmentReference> colorAttachmentReferences;
	std::vector<vk::AttachmentReference> resolveAttachmentReferences;
	std::optional<vk::AttachmentReference> depthStencilAttachmentReference;
	for (uint32_t index = 0; index < attachments.size(); index++)
	{
		vk::AttachmentDescription attachmentDescriptor;
		attachmentDescriptor.setFormat(attachments[index]->format)
			.setSamples(attachments[index]->flags)
			.setLoadOp(loadOp[index])
			.setStoreOp(storeOp[index])
			.setStencilLoadOp((attachments[index]->is_stencil ? loadOp[index] : vk::AttachmentLoadOp::eDontCare))
			.setStencilStoreOp((attachments[index]->is_stencil ? storeOp[index] : vk::AttachmentStoreOp::eDontCare))
			.setInitialLayout(attachments[index]->layout)
			.setFinalLayout(layout[index]);
		attachmentDescriptors.push_back(attachmentDescriptor);
		if (attachments[index]->is_stencil || attachments[index]->is_depth)
		{
			vk::AttachmentReference depthStencilAttachmentRef;
			depthStencilAttachmentRef.setAttachment(index)
				.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
			depthStencilAttachmentReference = depthStencilAttachmentRef;
		}
		else
		{
			vk::AttachmentReference colorAttachmentReference;
			colorAttachmentReference.setAttachment(index)
				.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
			colorAttachmentReferences.push_back(colorAttachmentReference);
		}
	}

	const uint32_t numAttachments = attachmentDescriptors.size();
	for (uint32_t index = 0; index < resolveAttachments.size(); index++)
	{
		vk::AttachmentDescription attachmentDescriptor;
		attachmentDescriptor.setFormat(resolveAttachments[index]->format)
			.setSamples(resolveAttachments[index]->flags)
			.setLoadOp(loadOp[index + numAttachments])
			.setStoreOp(storeOp[index + numAttachments])
			.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
			.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
			.setInitialLayout(resolveAttachments[index]->layout)
			.setFinalLayout(layout[index + numAttachments]);
		attachmentDescriptors.push_back(attachmentDescriptor);

		vk::AttachmentReference resolveAttachmentReference;
		resolveAttachmentReference.setAttachment(attachmentDescriptors.size() - 1)
			.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
		resolveAttachmentReferences.push_back(resolveAttachmentReference);
	}

	vk::SubpassDescription spd;
	spd.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
		.setColorAttachments(colorAttachmentReferences)
		.setPResolveAttachments(resolveAttachmentReferences.data())
		.setPDepthStencilAttachment(depthStencilAttachmentReference.has_value() ?
			&depthStencilAttachmentReference.value() : nullptr);

	std::array<vk::SubpassDependency, 2> dependencies;
	dependencies[0].setSrcSubpass(VK_SUBPASS_EXTERNAL)
		.setDstSubpass(0)
		.setSrcStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
		.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput |
			vk::PipelineStageFlagBits::eEarlyFragmentTests |
			vk::PipelineStageFlagBits::eLateFragmentTests |
			vk::PipelineStageFlagBits::eFragmentShader)
		.setSrcAccessMask(vk::AccessFlagBits::eMemoryRead)
		.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead |
			vk::AccessFlagBits::eColorAttachmentWrite |
			vk::AccessFlagBits::eDepthStencilAttachmentRead |
			vk::AccessFlagBits::eDepthStencilAttachmentWrite |
			vk::AccessFlagBits::eShaderRead);
	dependencies[1].setSrcSubpass(0)
		.setDstSubpass(VK_SUBPASS_EXTERNAL)
		.setSrcStageMask(
			vk::PipelineStageFlagBits::eColorAttachmentOutput |
			vk::PipelineStageFlagBits::eEarlyFragmentTests |
			vk::PipelineStageFlagBits::eLateFragmentTests)
		.setDstStageMask(vk::PipelineStageFlagBits::eAllCommands)
		.setSrcAccessMask(
			vk::AccessFlagBits::eColorAttachmentRead |
			vk::AccessFlagBits::eColorAttachmentWrite |
			vk::AccessFlagBits::eDepthStencilAttachmentRead |
			vk::AccessFlagBits::eDepthStencilAttachmentWrite)
		.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead |
			vk::AccessFlagBits::eColorAttachmentWrite |
			vk::AccessFlagBits::eDepthStencilAttachmentRead |
			vk::AccessFlagBits::eDepthStencilAttachmentWrite |
			vk::AccessFlagBits::eShaderRead);

	vk::RenderPassCreateInfo renderPassCI;
	renderPassCI.setAttachments(attachmentDescriptors)
		.setSubpasses(spd)
		.setDependencies(dependencies);
	 
	renderPass = Context::GetInstance().device.createRenderPass(renderPassCI);
}

RenderPass::RenderPass(const std::vector<vk::Format>& formats, 
	const std::vector<vk::ImageLayout>& initialLayouts, const std::vector<vk::ImageLayout>& finalLayouts, 
	const std::vector<vk::AttachmentLoadOp>& loadOp, const std::vector<vk::AttachmentStoreOp>& storeOp, 
	vk::PipelineBindPoint bindPoint, std::vector<uint32_t> resolveAttachmentsIndices, 
	uint32_t depthAttachmentIndex, uint32_t stencilAttachmentIndex, 
	vk::AttachmentLoadOp stencilLoadOp, vk::AttachmentStoreOp stencilStoreOp, bool multiView, const std::string& name)
{
	const bool sameSizes =
		formats.size() == initialLayouts.size() && formats.size() == finalLayouts.size() &&
		formats.size() == loadOp.size() && formats.size() == storeOp.size();
	if (!sameSizes) std::abort();

	std::vector<vk::AttachmentDescription> attachmentDescriptors;
	std::vector<vk::AttachmentReference> colorAttachmentReferences;
	std::optional<vk::AttachmentReference> depthStencilAttachmentReference;
	for (uint32_t index = 0; index < formats.size(); index++)
	{
		vk::AttachmentDescription attachmentDescriptor;
		attachmentDescriptor.setFormat(formats[index])
			.setSamples(vk::SampleCountFlagBits::e1)
			.setLoadOp(loadOp[index])
			.setStoreOp(storeOp[index])
			.setStencilLoadOp((index == stencilAttachmentIndex ? stencilLoadOp : vk::AttachmentLoadOp::eDontCare))
			.setStencilStoreOp((index == stencilAttachmentIndex ? stencilStoreOp : vk::AttachmentStoreOp::eDontCare))
			.setInitialLayout(initialLayouts[index])
			.setFinalLayout(finalLayouts[index]);
		attachmentDescriptors.push_back(attachmentDescriptor);
		if (index == depthAttachmentIndex || index == stencilAttachmentIndex)
		{
			vk::AttachmentReference depthStencilAttachmentRef;
			depthStencilAttachmentRef.setAttachment(index)
				.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
			depthStencilAttachmentReference = depthStencilAttachmentRef;
		}
		else
		{
			vk::AttachmentReference colorAttachmentReference;
			colorAttachmentReference.setAttachment(index)
				.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
			colorAttachmentReferences.push_back(colorAttachmentReference);
		}
	}

	vk::SubpassDescription spd;
	spd.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
		.setColorAttachments(colorAttachmentReferences)
		.setPDepthStencilAttachment((depthStencilAttachmentReference.has_value() ?
			&depthStencilAttachmentReference.value() : nullptr));

	std::array<vk::SubpassDependency, 2> dependencies;
	dependencies[0].setSrcSubpass(VK_SUBPASS_EXTERNAL)
		.setDstSubpass(0)
		.setSrcStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
		.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput |
			vk::PipelineStageFlagBits::eEarlyFragmentTests |
			vk::PipelineStageFlagBits::eLateFragmentTests |
			vk::PipelineStageFlagBits::eFragmentShader)
		.setSrcAccessMask(vk::AccessFlagBits::eMemoryRead)
		.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead |
			vk::AccessFlagBits::eColorAttachmentWrite |
			vk::AccessFlagBits::eDepthStencilAttachmentRead |
			vk::AccessFlagBits::eDepthStencilAttachmentWrite |
			vk::AccessFlagBits::eShaderRead);
	dependencies[1].setSrcSubpass(0)
		.setDstSubpass(VK_SUBPASS_EXTERNAL)
		.setSrcStageMask(
			vk::PipelineStageFlagBits::eColorAttachmentOutput |
			vk::PipelineStageFlagBits::eEarlyFragmentTests |
			vk::PipelineStageFlagBits::eLateFragmentTests)
		.setDstStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
		.setSrcAccessMask(
			vk::AccessFlagBits::eColorAttachmentRead |
			vk::AccessFlagBits::eColorAttachmentWrite |
			vk::AccessFlagBits::eDepthStencilAttachmentRead |
			vk::AccessFlagBits::eDepthStencilAttachmentWrite)
		.setDstAccessMask(vk::AccessFlagBits::eMemoryRead);

	uint32_t viewmask = 0x00000003;
	uint32_t correlationMask = 0x00000003;
	vk::RenderPassMultiviewCreateInfo mvci;
	mvci.setSubpassCount(1)
		.setPViewMasks(&viewmask)
		.setCorrelationMasks(correlationMask);

	vk::RenderPassCreateInfo rpci;
	rpci.setPNext((multiView ? &mvci : nullptr))
		.setAttachments(attachmentDescriptors)
		.setSubpasses(spd)
		.setDependencies(dependencies);

	renderPass = Context::GetInstance().device.createRenderPass(rpci);

}

RenderPass::RenderPass(const std::vector<vk::Format>& formats, const std::vector<vk::ImageLayout>& initialLayouts, const std::vector<vk::ImageLayout>& finalLayouts, const std::vector<vk::AttachmentLoadOp>& loadOp, const std::vector<vk::AttachmentStoreOp>& storeOp, vk::PipelineBindPoint bindPoint, std::vector<uint32_t> resolveAttachmentsIndices, uint32_t depthAttachmentIndex, uint32_t fragmentDensityMapIndex, uint32_t stencilAttachmentIndex, vk::AttachmentLoadOp stencilLoadOp, vk::AttachmentStoreOp stencilStoreOp, bool multiview, const std::string& name)
{
	const bool sameSizes =
		formats.size() == initialLayouts.size() && formats.size() == finalLayouts.size() &&
		formats.size() == loadOp.size() && formats.size() == storeOp.size();
	if (!sameSizes) std::abort();

	std::vector<vk::AttachmentDescription> attachmentDescriptors;
	std::vector<vk::AttachmentReference> colorAttachmentReferences;
	std::optional<vk::AttachmentReference> depthStencilAttachmentReference;
	uint32_t fragmentDensityAttachmentReference = UINT32_MAX;
	for (uint32_t index = 0; index < formats.size(); index++)
	{
		vk::AttachmentDescription attachmentDescriptor;
		attachmentDescriptor.setFormat(formats[index])
			.setSamples(vk::SampleCountFlagBits::e1)
			.setLoadOp(loadOp[index])
			.setStoreOp(storeOp[index])
			.setStencilLoadOp((index == stencilAttachmentIndex
				? stencilLoadOp : vk::AttachmentLoadOp::eDontCare))
			.setStencilStoreOp((index == stencilAttachmentIndex
				? stencilStoreOp : vk::AttachmentStoreOp::eDontCare))
			.setInitialLayout(initialLayouts[index])
			.setFinalLayout(finalLayouts[index]);

		if (index == depthAttachmentIndex || index == stencilAttachmentIndex) {
			vk::AttachmentReference depthStencilAttachmentRef;
			depthStencilAttachmentRef.setAttachment(index)
				.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
			depthStencilAttachmentReference = depthStencilAttachmentRef;
		}
		else if (index == fragmentDensityMapIndex) {
			fragmentDensityAttachmentReference = index;
		}
		else {
			vk::AttachmentReference colorAttachmentReference;
			colorAttachmentReference.setAttachment(index)
				.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
			colorAttachmentReferences.push_back(colorAttachmentReference);
		}
	}

#if defined(VK_EXT_fragment_density_map)
	vk::RenderPassFragmentDensityMapCreateInfoEXT fdmAttachmentCI;
	vk::AttachmentReference depthStencilAttachmentRef;
	depthStencilAttachmentRef.setAttachment(fragmentDensityAttachmentReference)
		.setLayout(vk::ImageLayout::eFragmentDensityMapOptimalEXT);
	fdmAttachmentCI.setFragmentDensityMapAttachment(depthStencilAttachmentRef);
#endif

	vk::SubpassDescription spd;
	spd.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
		.setColorAttachments(colorAttachmentReferences)
		.setPDepthStencilAttachment((depthStencilAttachmentReference.has_value() ?
			&depthStencilAttachmentReference.value() : nullptr));

	std::array<vk::SubpassDependency, 2> dependencies;
	dependencies[0].setSrcSubpass(VK_SUBPASS_EXTERNAL)
		.setDstSubpass(0)
		.setSrcStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
		.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput |
			vk::PipelineStageFlagBits::eEarlyFragmentTests |
			vk::PipelineStageFlagBits::eLateFragmentTests |
			vk::PipelineStageFlagBits::eFragmentShader)
		.setSrcAccessMask(vk::AccessFlagBits::eMemoryRead)
		.setDstAccessMask(
			vk::AccessFlagBits::eColorAttachmentRead |
			vk::AccessFlagBits::eColorAttachmentWrite |
			vk::AccessFlagBits::eDepthStencilAttachmentRead |
			vk::AccessFlagBits::eDepthStencilAttachmentWrite |
			vk::AccessFlagBits::eShaderRead);
	dependencies[1].setSrcSubpass(0)
		.setDstSubpass(VK_SUBPASS_EXTERNAL)
		.setSrcStageMask(
			vk::PipelineStageFlagBits::eColorAttachmentOutput |
			vk::PipelineStageFlagBits::eEarlyFragmentTests |
			vk::PipelineStageFlagBits::eLateFragmentTests)
		.setDstStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
		.setSrcAccessMask(
			vk::AccessFlagBits::eColorAttachmentRead |
			vk::AccessFlagBits::eColorAttachmentWrite |
			vk::AccessFlagBits::eDepthStencilAttachmentRead |
			vk::AccessFlagBits::eDepthStencilAttachmentWrite)
		.setDstAccessMask(vk::AccessFlagBits::eMemoryRead);

	uint32_t viewmask = 0x00000003;
	uint32_t correlationMask = 0x00000003;
	vk::RenderPassMultiviewCreateInfo mvci;
	mvci
#if defined(VK_EXT_fragment_density_map)
		.setPNext(fragmentDensityAttachmentReference < UINT32_MAX ? &fdmAttachmentCI : nullptr)
#else
		.setPNext(nullptr)
#endif
		.setSubpassCount(1)
		.setViewMasks(viewmask)
		.setCorrelationMasks(correlationMask);

	vk::RenderPassCreateInfo rpci;
	rpci.setPNext((multiview ? &mvci : nullptr))
		.setAttachments(attachmentDescriptors)
		.setSubpasses(spd)
		.setDependencies(dependencies);

	renderPass = Context::GetInstance().device.createRenderPass(rpci);
}

RenderPass::~RenderPass()
{
	Context::GetInstance().device.destroyRenderPass(renderPass);
}

vk::RenderPass RenderPass::vkRenderPass() const
{
	return renderPass;
}
