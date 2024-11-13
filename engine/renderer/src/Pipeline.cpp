#include "Pipeline.h"
#include "Context.h"


static constexpr int MAX_DESCRIPTOR_SETS = 4096 * 3;

#define ASSERT(expr, message) \
  {                           \
    void(message);            \
    assert(expr);             \
  }

Pipeline::Pipeline(const GraphicsPipelineDescriptor& desc, VkRenderPass renderPass, const std::string& _name)
	: graphicsPipelineDesc(desc), bindPoint(vk::PipelineBindPoint::eGraphics), m_vkRenderPass(renderPass), name(_name)
{
	createGraphicsPipeline();
}

Pipeline::Pipeline(const ComputePipelineDescriptor& desc, const std::string& _name)
	: computePipelineDesc(desc), bindPoint(vk::PipelineBindPoint::eCompute), name(_name)
{
	createComputePipeline();
}

Pipeline::Pipeline(const RayTracingPipelineDescriptor& desc, const std::string& _name)
	: rayTracingPipelineDesc(desc), bindPoint(vk::PipelineBindPoint::eRayTracingKHR), name(_name)
{
	createRayTracingPipeline();
}

Pipeline::~Pipeline()
{
	auto device = Context::GetInstance().device;
	device.destroyPipeline(m_pipeline);
	device.destroyPipelineLayout(m_pipelineLayout);
	device.destroyDescriptorPool(m_descPool);
	for (const auto& set : m_descriptorSets)
	{
		device.destroyDescriptorSetLayout(set.second.vkLayout);
	}
}

vk::Pipeline Pipeline::vkPipeline() const
{
	return m_pipeline;
}

vk::PipelineLayout Pipeline::vkPipelineLayout() const
{
	return m_pipelineLayout;
}

void Pipeline::updatePushConstant(vk::CommandBuffer cmdbuf, vk::ShaderStageFlags flags, uint32_t size, const void* data)
{
	cmdbuf.pushConstants(m_pipelineLayout, flags, 0, size, data);
}

void Pipeline::bind(vk::CommandBuffer cmdbuf)
{
	cmdbuf.bindPipeline(bindPoint, m_pipeline);
	updateDescriptorSets();
}

void Pipeline::bindVertexBuffer(vk::CommandBuffer cmdbuf, vk::Buffer vertexBuffer)
{
	std::array<vk::Buffer, 1> vertexBuffers{ vertexBuffer };
	std::array<vk::DeviceSize, 1> vertexOffset{ 0 };
	cmdbuf.bindVertexBuffers(0, vertexBuffers, vertexOffset);
}

void Pipeline::bindIndexBuffer(vk::CommandBuffer cmdbuf, vk::Buffer indexBuffer)
{
	cmdbuf.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);
}

void Pipeline::allocateDescriptors(const std::vector<SetAndCount>& setAndCount)
{
	if (!m_descPool)
	{
		initDescriptorPool();
	}

	for (auto set : setAndCount)
	{
		ASSERT(m_descriptorSets.contains(set.set), "This pipeline doesn't have a set with index " + std::to_string(set.set));
		vk::DescriptorSetAllocateInfo descSetAI;
		descSetAI.setDescriptorPool(m_descPool)
			.setDescriptorSetCount(1)
			.setSetLayouts(m_descriptorSets[set.set].vkLayout);
		for (size_t i = 0; i < set.count; i++)
		{
			vk::DescriptorSet descSet = Context::GetInstance().device.allocateDescriptorSets(descSetAI)[0];
			m_descriptorSets[set.set].vkSets.push_back(descSet);
		}
	}
}

void Pipeline::bindDescriptorSets(vk::CommandBuffer commandBuffer, const std::vector<SetAndBindingIndex>& sets)
{
	for (const auto& set : sets)
	{
		commandBuffer.bindDescriptorSets(bindPoint, m_pipelineLayout, set.set, m_descriptorSets[set.set].vkSets[set.bindIdx], {});
	}
}

void Pipeline::updateSamplersDescriptorSets(uint32_t set, uint32_t index, const std::vector<SetBindings>& bindings)
{
	ASSERT(!bindings.empty(), "bindings are empty");
	std::vector<std::vector<vk::DescriptorImageInfo>> samplerInfo(bindings.size());
	std::vector<vk::WriteDescriptorSet> writeDescSets; 
	writeDescSets.reserve(bindings.size());
	for (size_t idx = 0; auto & binding : bindings)
	{
		samplerInfo[idx].reserve(binding.samplers.size());
		for (const auto& sampler : binding.samplers)
		{
			vk::DescriptorImageInfo si;
			si.setSampler(sampler->vkSampler());
			samplerInfo[idx].push_back(si);
		}
		vk::WriteDescriptorSet writeDescSet;
		writeDescSet.setDstSet(m_descriptorSets[set].vkSets[index])
			.setDstBinding(binding.binding_)
			.setDstArrayElement(0)
			.setDescriptorCount(samplerInfo[idx].size())
			.setDescriptorType(vk::DescriptorType::eSampler)
			.setImageInfo(samplerInfo[idx]);
		writeDescSets.emplace_back(std::move(writeDescSet));
		++idx;
	}
	Context::GetInstance().device.updateDescriptorSets(writeDescSets, {});
}

void Pipeline::updateTexturesDescriptorSets(uint32_t set, uint32_t index, const std::vector<SetBindings>& bindings)
{
	ASSERT(!bindings.empty(), "bindings are empty");
	std::vector<std::vector<vk::DescriptorImageInfo>> imageInfo(bindings.size());
	std::vector<vk::WriteDescriptorSet> writeDescSets;
	writeDescSets.reserve(bindings.size());
	for (size_t idx = 0; auto & binding : bindings)
	{
		imageInfo[idx].reserve(binding.textures.size());
		for (const auto& texture : binding.textures)
		{
			vk::DescriptorImageInfo si;
			si.setImageView(texture->view)
				.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
			imageInfo[idx].push_back(si);
		}
		vk::WriteDescriptorSet writeDescSet;
		writeDescSet.setDstSet(m_descriptorSets[set].vkSets[index])
			.setDstBinding(binding.binding_)
			.setDstArrayElement(0)
			.setDescriptorCount(imageInfo[idx].size())
			.setDescriptorType(vk::DescriptorType::eSampledImage)
			.setImageInfo(imageInfo[idx]);
		writeDescSets.emplace_back(std::move(writeDescSet));
		++idx;
	}
	Context::GetInstance().device.updateDescriptorSets(writeDescSets, {});
}

void Pipeline::updateBuffersDescriptorSets(uint32_t set, uint32_t index, vk::DescriptorType type, const std::vector<SetBindings>& bindings)
{
	ASSERT(!bindings.empty(), "bindings are empty");
	std::vector<vk::DescriptorBufferInfo> bufferInfo;
	//bufferInfo.reserve(bindings.size());
	std::vector<vk::WriteDescriptorSet> writeDescSets;
	writeDescSets.reserve(bindings.size());

	for (auto& binding : bindings) {
		vk::DescriptorBufferInfo bi;
		bi.setBuffer(binding.buffer->buffer)
			.setOffset(0)
			.setRange(binding.bufferBytes);
		bufferInfo.push_back(bi);
	
		vk::WriteDescriptorSet writeDescSet;
		writeDescSet.setDstSet(m_descriptorSets[set].vkSets[index])
			.setDstBinding(binding.binding_)
			.setDstArrayElement(0)
			.setDescriptorCount(1)
			.setDescriptorType(type)
			.setBufferInfo(bufferInfo.back());

		writeDescSets.emplace_back(writeDescSet);
	}
	Context::GetInstance().device.updateDescriptorSets(writeDescSets, {});
}

void Pipeline::updateDescriptorSets()
{
	if (!m_writeDescSets.empty())
	{
		std::unique_lock<std::mutex> mLock(mutex);
		Context::GetInstance().device.updateDescriptorSets(m_writeDescSets, {});

		m_writeDescSets.clear();
		m_bufferInfo.clear();
		m_bufferViewInfo.clear();
		m_imageInfo.clear();
		accelerationStructInfo.clear();
	}
}

void Pipeline::bindResource(uint32_t set, uint32_t binding, uint32_t index, std::shared_ptr<Buffer> buffer, uint32_t offset, uint32_t size, vk::DescriptorType type, vk::Format format)
{
	vk::DescriptorBufferInfo bufferInfo;
	bufferInfo.setBuffer(buffer->buffer)
		.setOffset(offset)
		.setRange(size);
	m_bufferInfo.emplace_back(std::vector<vk::DescriptorBufferInfo>{bufferInfo});
	if (type == vk::DescriptorType::eStorageTexelBuffer || type == vk::DescriptorType::eUniformTexelBuffer)
	{
		ASSERT(format != vk::Format::eUndefined, "format must be specified");
		// TODO: Buffer类中没有设置vk::BufferView
		//bufferViewInfo_.emplace_back(buffer->requestBufferView(format));
	}
	ASSERT(m_descriptorSets[set].vkSets[index], "Did you allocate the descriptor set before binding to it?");
	
	vk::WriteDescriptorSet writeDescSet;
	writeDescSet.setDstSet(m_descriptorSets[set].vkSets[index])
		.setDstBinding(binding)
		.setDstArrayElement(0)
		.setDescriptorCount(1)
		.setDescriptorType(type);
	if (type == vk::DescriptorType::eStorageTexelBuffer || type == vk::DescriptorType::eUniformTexelBuffer)
	{
		writeDescSet.setTexelBufferView(m_bufferViewInfo.back());
	}
	else
	{
		writeDescSet.setBufferInfo(m_bufferInfo.back());
	}
	m_writeDescSets.emplace_back(std::move(writeDescSet));
}

void Pipeline::bindResource(uint32_t set, uint32_t binding, uint32_t index, std::span<std::shared_ptr<Texture>> textures, std::shared_ptr<Sampler> sampler, uint32_t dstArrayElement)
{
	if (textures.empty())
	{
		return;
	}
	std::unique_lock<std::mutex> mlock(mutex);
	std::vector<vk::DescriptorImageInfo> imageInfos;
	imageInfos.reserve(textures.size());
	for (const auto& texture : textures)
	{
		vk::DescriptorImageInfo imageInfo;
		imageInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
			.setImageView(texture->view);
		if (sampler)
		{
			imageInfo.setSampler(sampler->vkSampler());
		}
		imageInfos.emplace_back(imageInfo);
	}
	m_imageInfo.push_back(imageInfos);
	if (imageInfos.empty())
	{
		return;
	}
	ASSERT(m_descriptorSets[set].vkSets[index], "Did you allocate the descriptor set before binding to it?");

	vk::WriteDescriptorSet writeDescSet;
	writeDescSet.setDstSet(m_descriptorSets[set].vkSets[index])
		.setDstBinding(binding)
		.setDstArrayElement(dstArrayElement)
		.setDescriptorType((sampler ? vk::DescriptorType::eCombinedImageSampler : vk::DescriptorType::eSampledImage))
		.setDescriptorCount(imageInfos.size())
		.setImageInfo(m_imageInfo.back());

	m_writeDescSets.emplace_back(std::move(writeDescSet));
}

void Pipeline::bindResource(uint32_t set, uint32_t binding, uint32_t index, std::span<std::shared_ptr<Sampler>> samplers)
{
	std::vector<vk::DescriptorImageInfo> imageInfos;
	imageInfos.reserve(samplers.size());
	for (const auto& sampler : samplers)
	{
		vk::DescriptorImageInfo imageInfo;
		imageInfo.setSampler(sampler->vkSampler());
		imageInfos.emplace_back(imageInfo);
	}
	m_imageInfo.push_back(imageInfos);

	ASSERT(m_descriptorSets[set].vkSets[index], "Did you allocate the descriptor set before binding to it?");

	vk::WriteDescriptorSet writeDescSet;
	writeDescSet.setDstSet(m_descriptorSets[set].vkSets[index])
		.setDstBinding(binding)
		.setDstArrayElement(0)
		.setDescriptorType(vk::DescriptorType::eSampler)
		.setDescriptorCount(imageInfos.size())
		.setImageInfo(m_imageInfo.back());

	m_writeDescSets.emplace_back(std::move(writeDescSet));
}

void Pipeline::bindResource(uint32_t set, uint32_t binding, uint32_t index, std::span<vk::ImageView> imageViews, vk::DescriptorType type)
{
	std::vector<vk::DescriptorImageInfo> imageInfos;
	imageInfos.reserve(imageViews.size());
	for (const auto& imview : imageViews)
	{
		vk::DescriptorImageInfo imageInfo;
		imageInfo.setImageView(imview)
			.setImageLayout(vk::ImageLayout::eGeneral);
		imageInfos.push_back(imageInfo);
	}
	m_imageInfo.push_back(imageInfos);
	ASSERT(m_descriptorSets[set].vkSets[index],
		"Did you allocate the descriptor set before binding to it?");

	vk::WriteDescriptorSet writeDescSet;
	writeDescSet.setDstSet(m_descriptorSets[set].vkSets[index])
		.setDstBinding(binding)
		.setDstArrayElement(0)
		.setDescriptorCount(imageInfos.size())
		.setDescriptorType(type)
		.setImageInfo(m_imageInfo.back());

	m_writeDescSets.emplace_back(writeDescSet);
}

void Pipeline::bindResource(uint32_t set, uint32_t binding, uint32_t index, std::vector<std::shared_ptr<Buffer>> buffers, vk::DescriptorType type)
{
	std::vector<vk::DescriptorBufferInfo> bufferInfos;
	for (auto& buffer : buffers)
	{
		vk::DescriptorBufferInfo bufferInfo;
		bufferInfo.setBuffer(buffer->buffer)
			.setOffset(0)
			.setRange(buffer->size);
		bufferInfos.push_back(bufferInfo);
	}
	m_bufferInfo.push_back(bufferInfos);
	ASSERT(m_descriptorSets[set].vkSets[index],
		"Did you allocate the descriptor set before binding to it?");

	vk::WriteDescriptorSet writeDescSet;
	writeDescSet.setDstSet(m_descriptorSets[set].vkSets[index])
		.setDstBinding(binding)
		.setDstArrayElement(0)
		.setDescriptorCount(bufferInfos.size())
		.setDescriptorType(type)
		.setBufferInfo(m_bufferInfo.back());

	m_writeDescSets.emplace_back(std::move(writeDescSet));
}

void Pipeline::bindResource(uint32_t set, uint32_t binding, uint32_t index, std::shared_ptr<Texture> texture, vk::DescriptorType type)
{
	std::vector<vk::DescriptorImageInfo> imageInfos;
	vk::DescriptorImageInfo imageInfo;
	imageInfo.setImageView(texture->view)
		.setImageLayout(vk::ImageLayout::eGeneral);
	imageInfos.push_back(imageInfo);
	m_imageInfo.push_back(imageInfos);
	ASSERT(m_descriptorSets[set].vkSets[index],
		"Did you allocate the descriptor set before binding to it?");

	vk::WriteDescriptorSet writeDescSet;
	writeDescSet.setDstSet(m_descriptorSets[set].vkSets[index])
		.setDstBinding(binding)
		.setDstArrayElement(0)
		.setDescriptorCount(imageInfos.size())
		.setDescriptorType(type)
		.setImageInfo(m_imageInfo.back());
	m_writeDescSets.emplace_back(std::move(writeDescSet));
}

void Pipeline::bindResource(uint32_t set, uint32_t binding, uint32_t index, std::shared_ptr<Texture> texture, std::shared_ptr<Sampler> sampler, vk::DescriptorType type)
{
	std::vector<vk::DescriptorImageInfo> imageInfos;
	vk::DescriptorImageInfo imageInfo;
	imageInfo.setSampler(sampler->vkSampler())
		.setImageView(texture->view)
		.setImageLayout(vk::ImageLayout::eGeneral);
	imageInfos.push_back(imageInfo);
	m_imageInfo.push_back(imageInfos);
	ASSERT(m_descriptorSets[set].vkSets[index],
		"Did you allocate the descriptor set before binding to it?");

	vk::WriteDescriptorSet writeDescSet;
	writeDescSet.setDstSet(m_descriptorSets[set].vkSets[index])
		.setDstBinding(binding)
		.setDstArrayElement(0)
		.setDescriptorCount(imageInfos.size())
		.setDescriptorType(type)
		.setImageInfo(m_imageInfo.back());
	m_writeDescSets.emplace_back(std::move(writeDescSet));
}

void Pipeline::bindResource(uint32_t set, uint32_t binding, uint32_t index, vk::AccelerationStructureKHR* accelStructHandle)
{
	vk::WriteDescriptorSetAccelerationStructureKHR accelerationSI;
	accelerationSI.setAccelerationStructureCount(1)
		.setPAccelerationStructures(accelStructHandle);
	accelerationStructInfo.push_back(accelerationSI);
	ASSERT(m_descriptorSets[set].vkSets[index],
		"Did you allocate the descriptor set before binding to it?");

	vk::WriteDescriptorSet writeDescSet;
	writeDescSet.setPNext(&accelerationStructInfo.back())
		.setDstSet(m_descriptorSets[set].vkSets[index])
		.setDstBinding(binding)
		.setDstArrayElement(0)
		.setDescriptorCount(1u)
		.setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR);
	m_writeDescSets.emplace_back(writeDescSet);
}

void Pipeline::createGraphicsPipeline()
{
	vk::SpecializationInfo vertexSpecializationInfo;
	vertexSpecializationInfo.setMapEntries(graphicsPipelineDesc.vertexSpecConstants)
		.setDataSize((!graphicsPipelineDesc.vertexSpecConstants.empty() ?
			graphicsPipelineDesc.vertexSpecConstants.back().offset +
			graphicsPipelineDesc.vertexSpecConstants.back().size : 0))
		.setPData(graphicsPipelineDesc.vertexSpecializationData);

	vk::SpecializationInfo fragmentSpecializationInfo;
	fragmentSpecializationInfo.setMapEntries(graphicsPipelineDesc.fragmentSpecConstants)
		.setDataSize((!graphicsPipelineDesc.fragmentSpecConstants.empty() ?
			graphicsPipelineDesc.fragmentSpecConstants.back().offset +
			graphicsPipelineDesc.fragmentSpecConstants.back().size : 0))
		.setPData(graphicsPipelineDesc.fragmentSpecializationData);

	const auto vertShader = graphicsPipelineDesc.vertexShader.lock();
	const auto fragShader = graphicsPipelineDesc.fragmentShader.lock();
	ASSERT(vertShader, "Vertex's ShaderModule has been destroyed before being used to create a pipeline");
	ASSERT(fragShader, "Vertex's ShaderModule has been destroyed before being used to create a pipeline");

	std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages;
	shaderStages[0].setStage(vk::ShaderStageFlagBits::eVertex)
		.setModule(vertShader->module)
		.setPName(vertShader->name.c_str())
		.setPSpecializationInfo((!graphicsPipelineDesc.vertexSpecConstants.empty() ?
			&vertexSpecializationInfo : nullptr));
	shaderStages[1].setStage(vk::ShaderStageFlagBits::eFragment)
		.setModule(fragShader->module)
		.setPName(fragShader->name.c_str())
		.setPSpecializationInfo((!graphicsPipelineDesc.fragmentSpecConstants.empty() ?
			&fragmentSpecializationInfo : nullptr));

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
	inputAssembly.setTopology(graphicsPipelineDesc.primitiveTopology)
		.setPrimitiveRestartEnable(VK_FALSE);

	graphicsPipelineDesc.vertexInputCI.setVertexAttributeDescriptionCount(0)
		.setVertexBindingDescriptionCount(0);

	vk::Viewport viewport = graphicsPipelineDesc.viewport;
	vk::Rect2D scissor;
	scissor.setOffset(vk::Offset2D{ 0,0 })
		.setExtent(vk::Extent2D{ static_cast<uint32_t>(graphicsPipelineDesc.viewport.width), static_cast<uint32_t>(graphicsPipelineDesc.viewport.height) });
	vk::PipelineViewportStateCreateInfo viewportSCI;
	viewportSCI.setViewports(viewport)
		.setScissors(scissor);

	vk::PipelineRasterizationStateCreateInfo rasterizerSCI;
	rasterizerSCI.setDepthClampEnable(VK_FALSE)
		.setRasterizerDiscardEnable(VK_FALSE)
		.setPolygonMode(vk::PolygonMode::eFill)
		.setCullMode(graphicsPipelineDesc.cullMode)
		.setFrontFace(graphicsPipelineDesc.frontFace)
		.setDepthBiasEnable(VK_FALSE)
		.setDepthBiasConstantFactor(0.0f)
		.setDepthBiasClamp(0.0f)
		.setDepthBiasSlopeFactor(0.0f)
		.setLineWidth(1.0f);

	vk::PipelineMultisampleStateCreateInfo multisampleSCI;
	multisampleSCI.setRasterizationSamples(graphicsPipelineDesc.sampleCount)
		.setSampleShadingEnable(VK_FALSE)
		.setMinSampleShading(1.0f)
		.setAlphaToCoverageEnable(VK_FALSE)
		.setAlphaToOneEnable(VK_FALSE);

	std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments;
	if (graphicsPipelineDesc.blendAttachmentStates.size())
	{
		ASSERT(graphicsPipelineDesc.blendAttachmentStates.size() == graphicsPipelineDesc.colorTextureFormats.size(),
			"Blend states need to be provided for all color textures");
		colorBlendAttachments = graphicsPipelineDesc.blendAttachmentStates;
	}
	else
	{
		vk::PipelineColorBlendAttachmentState colorBlendAttachment;
		colorBlendAttachment.setBlendEnable(graphicsPipelineDesc.blendEnable)
			.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
			.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
			.setColorBlendOp(vk::BlendOp::eAdd)
			.setSrcAlphaBlendFactor(vk::BlendFactor::eSrcAlpha)
			.setDstAlphaBlendFactor(vk::BlendFactor::eDstAlpha)
			.setAlphaBlendOp(vk::BlendOp::eAdd)
			.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
				vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
		colorBlendAttachments = std::vector<vk::PipelineColorBlendAttachmentState>(graphicsPipelineDesc.colorTextureFormats.size(),
			colorBlendAttachment);
	}

	vk::PipelineColorBlendStateCreateInfo colorBlendSCI;
	colorBlendSCI.setLogicOpEnable(VK_FALSE)
		.setLogicOp(vk::LogicOp::eCopy)
		.setAttachments(colorBlendAttachments)
		.setBlendConstants({ 0.0f, 0.0f, 0.0f, 0.0f });

	// Descriptor Set
	initDescriptorLayout();

	// End of descriptor set layout
	// TODO: does order matter in descSetLayouts? YES!
	std::vector<vk::DescriptorSetLayout> descSetLayouts(m_descriptorSets.size());
	for (const auto& set : m_descriptorSets)
	{
		descSetLayouts[set.first] = set.second.vkLayout;
	}

	m_pipelineLayout = createPipelineLayout(descSetLayouts, graphicsPipelineDesc.pushConstants);

	vk::PipelineDepthStencilStateCreateInfo depthStencilSCI;
	depthStencilSCI.setDepthTestEnable(graphicsPipelineDesc.depthTestEnable)
		.setDepthWriteEnable(graphicsPipelineDesc.depthWriteEnable)
		.setDepthCompareOp(graphicsPipelineDesc.depthCompareOperation)
		.setDepthBoundsTestEnable(VK_FALSE)
		.setStencilTestEnable(VK_FALSE)
		.setFront({})
		.setBack({})
		.setMinDepthBounds(0.0f)
		.setMaxDepthBounds(1.0f);

	vk::PipelineDynamicStateCreateInfo dynamicSCI;
	dynamicSCI.setDynamicStates(graphicsPipelineDesc.dynamicStates);

	// only used for dynamic rendering
	vk::PipelineRenderingCreateInfo pipelineRenderCI;
	pipelineRenderCI.setColorAttachmentFormats(graphicsPipelineDesc.colorTextureFormats)
		.setDepthAttachmentFormat(graphicsPipelineDesc.depthTextureFormat)
		.setStencilAttachmentFormat(graphicsPipelineDesc.stencilTextureFormat);

	vk::GraphicsPipelineCreateInfo pipelineCI;
	pipelineCI.setPNext(graphicsPipelineDesc.useDynamicRendering ? &pipelineRenderCI : nullptr)
		.setStages(shaderStages)
		.setPVertexInputState(&graphicsPipelineDesc.vertexInputCI)
		.setPInputAssemblyState(&inputAssembly)
		.setPViewportState(&viewportSCI)
		.setPRasterizationState(&rasterizerSCI)
		.setPMultisampleState(&multisampleSCI)
		.setPDepthStencilState(&depthStencilSCI)
		.setPColorBlendState(&colorBlendSCI)
		.setPDynamicState(&dynamicSCI)
		.setLayout(m_pipelineLayout)
		.setRenderPass(m_vkRenderPass)
		.setBasePipelineHandle({})
		.setBasePipelineIndex(-1);

	auto result = Context::GetInstance().device.createGraphicsPipelines({}, pipelineCI);
	if (result.result != vk::Result::eSuccess)
	{

	}
	m_pipeline = result.value[0];
}

void Pipeline::createComputePipeline()
{
	const auto computeShader = computePipelineDesc.computerShader.lock();
	ASSERT(computeShader, "Compute's ShaderModule has been destroyed before being used to create a pipeline");

	vk::SpecializationInfo specializationInfo;
	specializationInfo.setMapEntries(computePipelineDesc.specializationConsts)
		.setDataSize((!computePipelineDesc.specializationConsts.empty() ?
			computePipelineDesc.specializationConsts.back().offset +
			computePipelineDesc.specializationConsts.back().size : 0))
		.setPData(computePipelineDesc.specializationData_);

	initDescriptorLayout();

	std::vector<vk::DescriptorSetLayout> descSetLayouts;
	for (const auto& set : m_descriptorSets)
	{
		descSetLayouts.push_back(set.second.vkLayout);
	}
	m_pipelineLayout = createPipelineLayout(descSetLayouts, computePipelineDesc.pushConstants);

	vk::PipelineShaderStageCreateInfo shaderStage;
	//TODO: 设置不同的stage
	shaderStage.setStage(vk::ShaderStageFlagBits::eCompute)
		.setModule(computeShader->module)
		.setPName(computeShader->name.c_str());

	vk::ComputePipelineCreateInfo computePipelineCI;
	computePipelineCI.setStage(shaderStage)
		.setLayout(m_pipelineLayout);

	auto result = Context::GetInstance().device.createComputePipelines({}, computePipelineCI);
	if (result.result != vk::Result::eSuccess)
	{
	}
	m_pipeline = result.value[0];
}

void Pipeline::createRayTracingPipeline()
{
	initDescriptorLayout();

	std::vector<vk::DescriptorSetLayout> descSetLayouts;
	for (const auto& set : m_descriptorSets)
	{
		descSetLayouts.push_back(set.second.vkLayout);
	}
	m_pipelineLayout = createPipelineLayout(descSetLayouts, rayTracingPipelineDesc.pushConstants);

	std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
	std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shaderGroups;

	const auto rayGenShader = rayTracingPipelineDesc.rayGenShader.lock();

	vk::PipelineShaderStageCreateInfo rayGenShaderInfo;
	rayGenShaderInfo.setStage(vk::ShaderStageFlagBits::eRaygenKHR)
		.setModule(rayGenShader->module)
		.setPName(rayGenShader->name.c_str());

	shaderStages.push_back(rayGenShaderInfo);

	vk::RayTracingShaderGroupCreateInfoKHR shaderGroup;
	shaderGroup.setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
		.setGeneralShader(shaderGroups.size() - 1)
		.setClosestHitShader(VK_SHADER_UNUSED_KHR)
		.setAnyHitShader(VK_SHADER_UNUSED_KHR)
		.setIntersectionShader(VK_SHADER_UNUSED_KHR);
	shaderGroups.push_back(shaderGroup);

	for (auto& rayMissShader : rayTracingPipelineDesc.rayMissShaders)
	{
		const auto rayMissShaderPtr = rayMissShader.lock();

		vk::PipelineShaderStageCreateInfo rayMissShaderInfo;
		rayMissShaderInfo.setStage(vk::ShaderStageFlagBits::eMissKHR)
			.setModule(rayMissShaderPtr->module)
			.setPName(rayMissShaderPtr->name.c_str());
		shaderStages.push_back(rayMissShaderInfo);

		vk::RayTracingShaderGroupCreateInfoKHR shaderGroup;
		shaderGroup.setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
			.setGeneralShader(shaderGroups.size() - 1)
			.setClosestHitShader(VK_SHADER_UNUSED_KHR)
			.setAnyHitShader(VK_SHADER_UNUSED_KHR)
			.setIntersectionShader(VK_SHADER_UNUSED_KHR);
		shaderGroups.push_back(shaderGroup);
	}

	for (auto& rayClosestHitShader : rayTracingPipelineDesc.rayClosestHitShaders)
	{
		const auto rayMissShaderPtr = rayClosestHitShader.lock();

		vk::PipelineShaderStageCreateInfo rayClosestHitShaderInfo;
		rayClosestHitShaderInfo.setStage(vk::ShaderStageFlagBits::eClosestHitKHR)
			.setModule(rayMissShaderPtr->module)
			.setPName(rayMissShaderPtr->name.c_str());
		shaderStages.push_back(rayClosestHitShaderInfo);

		vk::RayTracingShaderGroupCreateInfoKHR shaderGroup;
		shaderGroup.setType(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup)
			.setGeneralShader(VK_SHADER_UNUSED_KHR)
			.setClosestHitShader(shaderGroups.size() - 1)
			.setAnyHitShader(VK_SHADER_UNUSED_KHR)
			.setIntersectionShader(VK_SHADER_UNUSED_KHR);
		shaderGroups.push_back(shaderGroup);
	}

	vk::RayTracingPipelineCreateInfoKHR rayTracingPipelineInfo;
	rayTracingPipelineInfo.setStages(shaderStages)
		.setGroups(shaderGroups)
		.setMaxPipelineRayRecursionDepth(10)
		.setLayout(m_pipelineLayout);
	//auto vkCreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)vkGetDeviceProcAddr(Context::GetInstance().device, "vkCreateRayTracingPipelinesKHR");

	//动态加载
	PFN_vkGetInstanceProcAddr instance_proc = reinterpret_cast<PFN_vkGetInstanceProcAddr>(Context::GetInstance().instance.getProcAddr("vkGetInstanceProcAddr"));
	vk::DispatchLoaderDynamic dld(Context::GetInstance().instance, instance_proc);
	if (!dld.vkCreateRayTracingPipelinesKHR)
	{
	}
	//dls.vkCreateRayTracingPipelinesKHR
	auto result = Context::GetInstance().device.createRayTracingPipelinesKHR({}, {}, rayTracingPipelineInfo, nullptr, dld);
	if (result.result != vk::Result::eSuccess)
	{
	}
	m_pipeline = result.value[0];
}

vk::PipelineLayout Pipeline::createPipelineLayout(const std::vector<vk::DescriptorSetLayout>& descLayouts, const std::vector<vk::PushConstantRange>& pushConsts) const
{
	vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
	pipelineLayoutInfo.setSetLayouts(descLayouts)
		.setPushConstantRanges(pushConsts);
	vk::PipelineLayout pipelineLayout;
	pipelineLayout = Context::GetInstance().device.createPipelineLayout(pipelineLayoutInfo);
	return pipelineLayout;
}

void Pipeline::initDescriptorPool()
{
	std::vector<SetDescriptor> sets;
	if (bindPoint == vk::PipelineBindPoint::eGraphics)
	{
		sets = graphicsPipelineDesc.sets;
	}
	else if (bindPoint == vk::PipelineBindPoint::eCompute)
	{
		sets = computePipelineDesc.sets;
	}
	else if (bindPoint == vk::PipelineBindPoint::eRayTracingKHR)
	{
		sets = rayTracingPipelineDesc.sets;
	}

	std::vector<vk::DescriptorPoolSize> poolSizes;
	for (size_t setIndex = 0; const auto& set : sets)
	{
		for (const auto& binding : set.bindings)
		{
			poolSizes.push_back({ binding.descriptorType, MAX_DESCRIPTOR_SETS });
		}
	}

	vk::DescriptorPoolCreateInfo descriptorPoolInfo;
	descriptorPoolInfo.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet |
		vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind)
		.setMaxSets(MAX_DESCRIPTOR_SETS)
		.setPoolSizes(poolSizes);
	
	m_descPool = Context::GetInstance().device.createDescriptorPool(descriptorPoolInfo);
}

void Pipeline::initDescriptorLayout()
{
	std::vector<SetDescriptor> sets;
	if (bindPoint == vk::PipelineBindPoint::eGraphics)
	{
		sets = graphicsPipelineDesc.sets;
	}
	else if (bindPoint == vk::PipelineBindPoint::eCompute)
	{
		sets = computePipelineDesc.sets;
	}
	else if (bindPoint == vk::PipelineBindPoint::eRayTracingKHR)
	{
		sets = rayTracingPipelineDesc.sets;
	}
	constexpr vk::DescriptorBindingFlags flagsToEnable =
		vk::DescriptorBindingFlagBits::ePartiallyBound |
		vk::DescriptorBindingFlagBits::eUpdateUnusedWhilePending;

	for (size_t setIndex = 0; const auto & set : sets)
	{
		std::vector<vk::DescriptorBindingFlags> bindFlags(set.bindings.size(), flagsToEnable);
		/* this won't work for android */
		vk::DescriptorSetLayoutBindingFlagsCreateInfo extendedInfo;
		extendedInfo.setBindingFlags(bindFlags);
		/* end of not working for android */

		vk::DescriptorSetLayoutCreateInfo dslci;
		dslci.setBindings(set.bindings);
		/* the next two lines won't work for android */
#if defined(_WIN32)
		dslci.setPNext(&extendedInfo)
			.setFlags(vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPoolEXT);
#endif
		/* end of not working for android*/
		vk::DescriptorSetLayout descriptorSetLayout = Context::GetInstance().device.createDescriptorSetLayout(dslci);
		m_descriptorSets[set.set].vkLayout = descriptorSetLayout;
	}
}
