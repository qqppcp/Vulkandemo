#pragma once

#include <vulkan/vulkan.hpp>
#include <memory>
#include <unordered_map>
#include <mutex>

#include "Shader.h"
#include "Texture.h"
#include "Sampler.h"
#include "Buffer.h"

class Pipeline final
{
public:
	struct SetDescriptor {
		uint32_t set;
		std::vector<vk::DescriptorSetLayoutBinding> bindings;
	};

	struct GraphicsPipelineDescriptor
	{
		std::vector<SetDescriptor> sets;
		std::weak_ptr<Shader> vertexShader;
		std::weak_ptr<Shader> fragmentShader;
		std::vector<vk::PushConstantRange> pushConstants;
		std::vector<vk::DynamicState> dynamicStates;
		bool useDynamicRendering = false;
		std::vector<vk::Format> colorTextureFormats;
		vk::Format depthTextureFormat = vk::Format::eUndefined;
		vk::Format stencilTextureFormat = vk::Format::eUndefined;
		vk::PolygonMode polygonMode = vk::PolygonMode::eFill;

		vk::PrimitiveTopology primitiveTopology = vk::PrimitiveTopology::eTriangleList;
		vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1;
		vk::CullModeFlagBits cullMode = vk::CullModeFlagBits::eBack;
		vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise;
		vk::Viewport viewport;
		bool blendEnable = false;
		uint32_t numberBlendAttachments = 0u;
		bool depthTestEnable = true;
		bool depthWriteEnable = true;
		vk::CompareOp depthCompareOperation = vk::CompareOp::eLess;
		vk::PipelineVertexInputStateCreateInfo vertexInputCI;
		std::vector<vk::SpecializationMapEntry> vertexSpecConstants;
		std::vector<vk::SpecializationMapEntry> fragmentSpecConstants;
		void* vertexSpecializationData = nullptr;
		void* fragmentSpecializationData = nullptr;

		std::vector<vk::PipelineColorBlendAttachmentState> blendAttachmentStates;
	};

	struct ComputePipelineDescriptor
	{
		std::vector<SetDescriptor> sets;
		std::weak_ptr<Shader> computerShader;
		std::vector<vk::PushConstantRange> pushConstants;
		std::vector<vk::SpecializationMapEntry> specializationConsts;
		void* specializationData_ = nullptr;
	};

	struct RayTracingPipelineDescriptor
	{
		std::vector<SetDescriptor> sets;
		std::weak_ptr<Shader> rayGenShader;
		std::vector<std::weak_ptr<Shader>> rayMissShaders;
		std::vector<std::weak_ptr<Shader>> rayClosestHitShaders;
		std::vector<vk::PushConstantRange> pushConstants;
		// Add specialization const, but they are needed per shaderModule?
	};

	explicit Pipeline(const GraphicsPipelineDescriptor& desc, VkRenderPass renderPass, const std::string& name = "");
	explicit Pipeline(const ComputePipelineDescriptor& desc, const std::string& name = "");
	explicit Pipeline(const RayTracingPipelineDescriptor& desc, const std::string& name = "");

	~Pipeline();

	bool valid() const { return !m_pipeline; }

	vk::Pipeline vkPipeline() const;
	vk::PipelineLayout vkPipelineLayout() const;
	void updatePushConstant(vk::CommandBuffer cmdbuf, vk::ShaderStageFlags flags, uint32_t size, const void* data);
	void bind(vk::CommandBuffer cmdbuf);
	void bindVertexBuffer(vk::CommandBuffer cmdbuf, vk::Buffer vertexBuffer);
	void bindIndexBuffer(vk::CommandBuffer cmdbuf, vk::Buffer indexBuffer);

	struct SetAndCount
	{
		uint32_t set;
		uint32_t count;
		std::string name;
	};
	void allocateDescriptors(const std::vector<SetAndCount>& setAndCount);

	struct SetAndBindingIndex
	{
		uint32_t set;
		uint32_t bindIdx;
	};
	void bindDescriptorSets(vk::CommandBuffer commandBuffer, const std::vector<SetAndBindingIndex>& sets);

	struct SetBindings {
		uint32_t set_ = 0;
		uint32_t binding_ = 0;
		std::span<std::shared_ptr<Texture>> textures;
		std::span<std::shared_ptr<Sampler>> samplers;
		std::shared_ptr<Buffer> buffer;
		uint32_t index_ = 0;
		uint32_t offset_ = 0;
		VkDeviceSize bufferBytes = 0;
	};
	void updateSamplersDescriptorSets(uint32_t set, uint32_t index, const std::vector<SetBindings>& bindings);
	void updateTexturesDescriptorSets(uint32_t set, uint32_t index, const std::vector<SetBindings>& bindings);
	void updateBuffersDescriptorSets(uint32_t set, uint32_t index, vk::DescriptorType type, const std::vector<SetBindings>& bindings);
	void updateDescriptorSets();

	/// @brief Assigns the resource to a position in the resource array specific to te resource's type
	void bindResource(uint32_t set, uint32_t binding, uint32_t index,
		std::shared_ptr<Buffer> buffer, uint32_t offset, uint32_t size,
		vk::DescriptorType type, vk::Format format = vk::Format::eUndefined);
	// if sampler is passed, it applies to all textures
	void bindResource(uint32_t set, uint32_t binding, uint32_t index,
		std::span<std::shared_ptr<Texture>> textures,
		std::shared_ptr<Sampler> sampler = nullptr,
		uint32_t dstArrayElement = 0);
	void bindResource(uint32_t set, uint32_t binding, uint32_t index,
		std::span<std::shared_ptr<Sampler>> samplers);
	void bindResource(uint32_t set, uint32_t binding, uint32_t index,
		std::span<vk::ImageView> imageViews,
		vk::DescriptorType type);
	void bindResource(uint32_t set, uint32_t binding, uint32_t index,
		std::vector<std::shared_ptr<Buffer>> buffers, vk::DescriptorType type);
	void bindResource(uint32_t set, uint32_t binding, uint32_t index,
		std::shared_ptr<Texture> texture, vk::DescriptorType type);
	void bindResource(uint32_t set, uint32_t binding, uint32_t index,
		std::shared_ptr<Texture> texture, std::shared_ptr<Sampler> sampler,
		vk::DescriptorType type = vk::DescriptorType::eCombinedImageSampler);
	void bindResource(uint32_t set, uint32_t binding, uint32_t index,
		vk::AccelerationStructureKHR* accelStructHandle);

private:
	void createGraphicsPipeline();

	void createComputePipeline();

	void createRayTracingPipeline();

	vk::PipelineLayout createPipelineLayout(
		const std::vector<vk::DescriptorSetLayout>& descLayouts,
		const std::vector<vk::PushConstantRange>& pushConsts) const;

	void initDescriptorPool();
	void initDescriptorLayout();

private:
	std::string name;
	GraphicsPipelineDescriptor graphicsPipelineDesc;
	ComputePipelineDescriptor computePipelineDesc;
	RayTracingPipelineDescriptor rayTracingPipelineDesc;
	vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eGraphics;
	vk::Pipeline m_pipeline = VK_NULL_HANDLE;
	vk::PipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
	vk::RenderPass m_vkRenderPass = VK_NULL_HANDLE;

	struct DescriptorSet
	{
		std::vector<vk::DescriptorSet> vkSets;
		vk::DescriptorSetLayout vkLayout = VK_NULL_HANDLE;
	};
	std::unordered_map<uint32_t, DescriptorSet> m_descriptorSets;
	vk::DescriptorPool m_descPool = VK_NULL_HANDLE;
	std::vector<vk::PushConstantRange> m_pushConsts;

	std::list<std::vector<vk::DescriptorBufferInfo>> m_bufferInfo;
	std::list<vk::BufferView> m_bufferViewInfo;
	std::list<std::vector<vk::DescriptorImageInfo>> m_imageInfo;
	std::vector<vk::WriteDescriptorSetAccelerationStructureKHR> accelerationStructInfo;
	std::vector<vk::WriteDescriptorSet> m_writeDescSets;
	std::mutex mutex;
};