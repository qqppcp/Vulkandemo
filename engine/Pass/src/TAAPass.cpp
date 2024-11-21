#include "TAAPass.h"
#include "Context.h"
#include "Sampler.h"
#include "Pipeline.h"
#include "program.h"
#include "Texture.h"
#include "define.h"

constexpr uint32_t OUTPUT_IMG_SET = 0;
constexpr uint32_t OUTPUT_IMAGE_BINDING = 0;

constexpr uint32_t INPUT_DATA_SET = 1;
constexpr uint32_t INPUT_DEPTH_BUFFER_BINDING = 0;
constexpr uint32_t INPUT_HISTORY_BUFFER_BINDING = 1;
constexpr uint32_t INPUT_VELOCITY_BUFFER_BINDING = 2;
constexpr uint32_t INPUT_COLOR_BUFFER_BINDING = 3;

void TAAPass::init(std::shared_ptr<Texture> depthTexture, std::shared_ptr<Texture> velocityTexture, std::shared_ptr<Texture> colorTexture)
{
	width = depthTexture->width;
	height = depthTexture->height;
	this->depthTexture = depthTexture;
	this->velocityTexture = velocityTexture;
	this->colorTexture = colorTexture;
	sampler.reset(new Sampler(vk::Filter::eLinear, vk::Filter::eLinear,
		vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge,
		vk::SamplerAddressMode::eClampToEdge, 100.0f));
	pointSampler.reset(new Sampler(vk::Filter::eNearest, vk::Filter::eNearest,
		vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge,
		vk::SamplerAddressMode::eClampToEdge, 100.0f));

	outColorTexture = TextureManager::Instance().Create(width, height, vk::Format::eR16G16B16A16Sfloat,
		vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage, getMipLevelsCount(width, height));
	historyTexture = TextureManager::Instance().Create(width, height, vk::Format::eR16G16B16A16Sfloat,
		vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage, getMipLevelsCount(width, height));

	auto shader = std::make_shared<GPUProgram>(shaderPath + "taaresolve.comp");

	std::vector<Pipeline::SetDescriptor> setLayouts;
	{
		Pipeline::SetDescriptor set;
		set.set = OUTPUT_IMG_SET;
		vk::DescriptorSetLayoutBinding binding;
		binding.setBinding(OUTPUT_IMAGE_BINDING)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eStorageImage)
			.setStageFlags(vk::ShaderStageFlagBits::eCompute);
		set.bindings.push_back(binding);
		setLayouts.push_back(set);
	}
	{
		Pipeline::SetDescriptor set;
		set.set = INPUT_DATA_SET;
		vk::DescriptorSetLayoutBinding binding;
		binding.setBinding(INPUT_DEPTH_BUFFER_BINDING)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setStageFlags(vk::ShaderStageFlagBits::eCompute);
		set.bindings.push_back(binding);
		binding.setBinding(INPUT_HISTORY_BUFFER_BINDING)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setStageFlags(vk::ShaderStageFlagBits::eCompute);
		set.bindings.push_back(binding);
		binding.setBinding(INPUT_VELOCITY_BUFFER_BINDING)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setStageFlags(vk::ShaderStageFlagBits::eCompute);
		set.bindings.push_back(binding);
		binding.setBinding(INPUT_COLOR_BUFFER_BINDING)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setStageFlags(vk::ShaderStageFlagBits::eCompute);
		set.bindings.push_back(binding);
		setLayouts.push_back(set);
	}
	std::vector<vk::PushConstantRange> ranges(1);
	ranges[0].setOffset(0)
		.setSize(sizeof(TAAPushConstants))
		.setStageFlags(vk::ShaderStageFlagBits::eCompute);
	const Pipeline::ComputePipelineDescriptor desc = {
		.sets = setLayouts,
		.computerShader = shader->Compute,
		.pushConstants = ranges,
	};
	pipeline.reset(new Pipeline(desc));
	pipeline->allocateDescriptors({
		{.set = OUTPUT_IMG_SET, .count = 1},
		{.set = INPUT_DATA_SET, .count = 1},
		});
	pipeline->bindResource(OUTPUT_IMG_SET, OUTPUT_IMAGE_BINDING, 0, outColorTexture, vk::DescriptorType::eStorageImage);
	pipeline->bindResource(INPUT_DATA_SET, INPUT_DEPTH_BUFFER_BINDING, 0, this->depthTexture, pointSampler);
	pipeline->bindResource(INPUT_DATA_SET, INPUT_HISTORY_BUFFER_BINDING, 0, historyTexture, sampler);
	pipeline->bindResource(INPUT_DATA_SET, INPUT_VELOCITY_BUFFER_BINDING, 0, this->velocityTexture, sampler);
	pipeline->bindResource(INPUT_DATA_SET, INPUT_COLOR_BUFFER_BINDING, 0, this->colorTexture, pointSampler);
	
	initSharpenPipeline();
}

void TAAPass::doAA(vk::CommandBuffer cmdbuf, uint32_t index, int isCamMoving)
{
	TAAPushConstants pushConst{
		.isFirstFrame = index,
		.isCameraMoving = uint32_t(isCamMoving),
	};

	pipeline->bind(cmdbuf);
	pipeline->updatePushConstant(cmdbuf, vk::ShaderStageFlagBits::eCompute, sizeof(TAAPushConstants), &pushConst);
	pipeline->bindDescriptorSets(cmdbuf, {
		{.set = OUTPUT_IMG_SET, .bindIdx = 0},
		{.set = INPUT_DATA_SET, .bindIdx = 0},
		});
	pipeline->updateDescriptorSets();
	outColorTexture->transitionImageLayout(cmdbuf, vk::ImageLayout::eGeneral);
	historyTexture->transitionImageLayout(cmdbuf, vk::ImageLayout::eGeneral);
	cmdbuf.dispatch((outColorTexture->width + 15) / 16, (outColorTexture->height + 15) / 16, 1);

	vk::ImageSubresourceRange range;
	range.setAspectMask(vk::ImageAspectFlagBits::eColor)
		.setLayerCount(1)
		.setLevelCount(1);
	vk::ImageMemoryBarrier barriers[2];
	barriers[0].setSrcAccessMask(vk::AccessFlagBits::eShaderWrite)
		.setDstAccessMask(vk::AccessFlagBits::eShaderRead)
		.setOldLayout(vk::ImageLayout::eGeneral)
		.setNewLayout(vk::ImageLayout::eGeneral)
		.setImage(outColorTexture->image)
		.setSubresourceRange(range);
	barriers[1].setSrcAccessMask(vk::AccessFlagBits::eShaderWrite)
		.setDstAccessMask(vk::AccessFlagBits::eShaderRead)
		.setOldLayout(vk::ImageLayout::eGeneral)
		.setNewLayout(vk::ImageLayout::eGeneral)
		.setImage(historyTexture->image)
		.setSubresourceRange(range);
	cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader,
		{}, {}, {}, barriers);
	colorTexture->transitionImageLayout(cmdbuf, vk::ImageLayout::eGeneral);
	sharpenPipeline->bind(cmdbuf);
	sharpenPipeline->bindDescriptorSets(cmdbuf, {
		{.set = 0, .bindIdx = 0},
		{.set = 1, .bindIdx = 0},
		});
	sharpenPipeline->updateDescriptorSets();
	cmdbuf.dispatch((outColorTexture->width + 15) / 16, (outColorTexture->height + 15) / 16, 1);
	colorTexture->transitionImageLayout(cmdbuf, vk::ImageLayout::eShaderReadOnlyOptimal);
	outColorTexture->transitionImageLayout(cmdbuf, vk::ImageLayout::eShaderReadOnlyOptimal);
}

void TAAPass::initSharpenPipeline()
{
	auto shader = std::make_shared<GPUProgram>(shaderPath + "taahistorycopyandsharpen.comp");
	std::vector<Pipeline::SetDescriptor> setLayouts;
	{
		Pipeline::SetDescriptor set;
		set.set = 0;
		vk::DescriptorSetLayoutBinding binding;
		binding.setBinding(0)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eStorageImage)
			.setStageFlags(vk::ShaderStageFlagBits::eCompute);
		set.bindings.push_back(binding);
		binding.setBinding(1)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eStorageImage)
			.setStageFlags(vk::ShaderStageFlagBits::eCompute);
		set.bindings.push_back(binding);
		setLayouts.push_back(set);
	}
	{
		Pipeline::SetDescriptor set;
		set.set = 1;
		vk::DescriptorSetLayoutBinding binding;
		binding.setBinding(0)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eStorageImage)
			.setStageFlags(vk::ShaderStageFlagBits::eCompute);
		set.bindings.push_back(binding);
		setLayouts.push_back(set);
	}
	const Pipeline::ComputePipelineDescriptor desc = {
		.sets = setLayouts,
		.computerShader = shader->Compute,
	};
	sharpenPipeline.reset(new Pipeline(desc));
	sharpenPipeline->allocateDescriptors({
		{.set = 0, .count = 1},
		{.set = 1, .count = 1},
		});
	sharpenPipeline->bindResource(0, 0, 0, colorTexture, vk::DescriptorType::eStorageImage);
	sharpenPipeline->bindResource(0, 1, 0, historyTexture, vk::DescriptorType::eStorageImage);
	sharpenPipeline->bindResource(1, 0, 0, outColorTexture, vk::DescriptorType::eStorageImage);
}
