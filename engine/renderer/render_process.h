#pragma once

#include "vulkan/vulkan.hpp"

class GPUProgram;

class Texture;

class RenderProcess final {
public:
	vk::Pipeline pipeline;
	vk::PipelineLayout layout;
	vk::RenderPass renderPass;

	std::array<vk::DescriptorSetLayout, 2> setlayouts;
	void InitPipelineLayout();
	void InitRenderPass();
	void InitPipeline(GPUProgram*);
	~RenderProcess();
	void DestroyPipeline();

};

class RenderPass final
{
public:

	RenderPass(const std::vector<std::shared_ptr<Texture>> attachments,
		const std::vector<std::shared_ptr<Texture>> resolveAttachments,
		const std::vector<vk::AttachmentLoadOp>& loadOp,
		const std::vector<vk::AttachmentStoreOp>& storeOp,
		const std::vector<vk::ImageLayout>& layout, vk::PipelineBindPoint bindPoint,
		const std::string& name = "");

	RenderPass(const std::vector<vk::Format>& formats,
		const std::vector<vk::ImageLayout>& initialLayouts,
		const std::vector<vk::ImageLayout>& finalLayouts,
		const std::vector<vk::AttachmentLoadOp>& loadOp,
		const std::vector<vk::AttachmentStoreOp>& storeOp,
		vk::PipelineBindPoint bindPoint,
		std::vector<uint32_t> resolveAttachmentsIndices,
		uint32_t depthAttachmentIndex, uint32_t stencilAttachmentIndex = UINT32_MAX,
		vk::AttachmentLoadOp stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
		vk::AttachmentStoreOp stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
		bool multiView = false,
		const std::string& name = "");

	// RenderPass2 - Fragment Density Map support
	RenderPass(const std::vector<vk::Format>& formats,
		const std::vector<vk::ImageLayout>& initialLayouts,
		const std::vector<vk::ImageLayout>& finalLayouts,
		const std::vector<vk::AttachmentLoadOp>& loadOp,
		const std::vector<vk::AttachmentStoreOp>& storeOp,
		vk::PipelineBindPoint bindPoint,
		std::vector<uint32_t> resolveAttachmentsIndices,
		uint32_t depthAttachmentIndex, uint32_t fragmentDensityMapIndex,
		uint32_t stencilAttachmentIndex = UINT32_MAX,
		vk::AttachmentLoadOp stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
		vk::AttachmentStoreOp stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
		bool multiview = false, const std::string& name = "");

	~RenderPass();

	vk::RenderPass vkRenderPass() const;

private:
	vk::RenderPass renderPass;
};