#include "HierarchicalDepthBufferPass.h"
#include "Texture.h"
#include "Pipeline.h"
#include "Context.h"
#include "Sampler.h"
#include "program.h"
#include "define.h"
#include <glm/glm.hpp>

constexpr uint32_t HIERARCHICALDEPTH_SET = 0;
constexpr uint32_t BINDING_OUT_HIERARCHICAL_DEPTH_TEXTURE = 0;
constexpr uint32_t BINDING_DEPTH_TEXTURE = 1;
constexpr uint32_t BINDING_PREV_HIERARCHICAL_DEPTH_TEXTURE = 2;

namespace
{
	int miplevels;
}

struct HierarchicalDepthPushConst {
	glm::uvec2 currentMipDimensions;
	glm::uvec2 prevMipDimensions;
	int32_t mipLevelIndex;
};

HierarchicalDepthBufferPass::HierarchicalDepthBufferPass() {}

HierarchicalDepthBufferPass::~HierarchicalDepthBufferPass()
{
	for (auto& imageView : hierarchicalDepthTexturePerMipImageViews)
	{
		Context::GetInstance().device.destroyImageView(imageView);
	}
}

void HierarchicalDepthBufferPass::init(std::shared_ptr<Texture> depthTexture)
{
	this->depthTexture = depthTexture;
	sampler.reset(new Sampler(vk::Filter::eLinear, vk::Filter::eLinear,
		vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge,
		vk::SamplerAddressMode::eClampToEdge, 100.0f));
	auto width = Context::GetInstance().swapchain->info.imageExtent.width;
	auto height = Context::GetInstance().swapchain->info.imageExtent.height;
	miplevels = getMipLevelsCount(width, height);
	outHierarchicalDepthTexture = TextureManager::Instance().Create(width, height, vk::Format::eR32G32Sfloat,
		vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage, miplevels);
	outHierarchicalDepthTexture->layout = vk::ImageLayout::eUndefined;

	std::shared_ptr<GPUProgram> shader = std::make_shared<GPUProgram>(shaderPath + "hizgen.comp.spv");
	std::vector<Pipeline::SetDescriptor> setLayouts;
	{
		Pipeline::SetDescriptor set;
		set.set = HIERARCHICALDEPTH_SET;
		vk::DescriptorSetLayoutBinding binding;
		binding.setBinding(BINDING_OUT_HIERARCHICAL_DEPTH_TEXTURE)
			.setDescriptorCount(miplevels)
			.setDescriptorType(vk::DescriptorType::eStorageImage)
			.setStageFlags(vk::ShaderStageFlagBits::eCompute);
		set.bindings.push_back(binding);
		binding.setBinding(BINDING_DEPTH_TEXTURE)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setStageFlags(vk::ShaderStageFlagBits::eCompute);
		set.bindings.push_back(binding);
		binding.setBinding(BINDING_PREV_HIERARCHICAL_DEPTH_TEXTURE)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setStageFlags(vk::ShaderStageFlagBits::eCompute);
		set.bindings.push_back(binding);
		setLayouts.push_back(set);
	}
	std::vector<vk::PushConstantRange> ranges(1);
	ranges[0].setOffset(0)
		.setSize(sizeof(HierarchicalDepthPushConst))
		.setStageFlags(vk::ShaderStageFlagBits::eCompute);

	vk::SpecializationMapEntry specializationMap;
	specializationMap.setConstantID(0)
		.setOffset(0)
		.setSize(sizeof(int32_t));

	const Pipeline::ComputePipelineDescriptor desc = {
		.sets = setLayouts,
		.computerShader = shader->Compute,
		.pushConstants = ranges,
		.specializationConsts = { specializationMap },
		.specializationData_ = &miplevels,
	};
	m_pipeline.reset(new Pipeline(desc));
	m_pipeline->allocateDescriptors({
		{.set = HIERARCHICALDEPTH_SET, .count = 1}
		});

	hierarchicalDepthTexturePerMipImageViews.reserve(miplevels);

	for (int i = 0; i < miplevels; ++i) {
		vk::ComponentMapping mapping;
		mapping.setR(vk::ComponentSwizzle::eIdentity)
			.setG(vk::ComponentSwizzle::eIdentity)
			.setB(vk::ComponentSwizzle::eIdentity)
			.setA(vk::ComponentSwizzle::eIdentity);
		vk::ImageSubresourceRange imageRange;
		imageRange.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setBaseMipLevel(uint32_t(i))
			.setLevelCount(1)
			.setBaseArrayLayer(0)
			.setLayerCount(1);
		vk::ImageViewCreateInfo imageViewInfo;
		imageViewInfo.setImage(outHierarchicalDepthTexture->image)
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(vk::Format::eR32G32Sfloat)
			.setComponents(mapping)
			.setSubresourceRange(imageRange);

		hierarchicalDepthTexturePerMipImageViews.push_back(Context::GetInstance().device.createImageView(imageViewInfo));

	}
	m_pipeline->bindResource(HIERARCHICALDEPTH_SET, BINDING_OUT_HIERARCHICAL_DEPTH_TEXTURE,
		0, std::span<vk::ImageView>(hierarchicalDepthTexturePerMipImageViews), 
		vk::DescriptorType::eStorageImage);
	m_pipeline->bindResource(HIERARCHICALDEPTH_SET, BINDING_DEPTH_TEXTURE, 0, depthTexture, sampler);
	m_pipeline->bindResource(HIERARCHICALDEPTH_SET, BINDING_PREV_HIERARCHICAL_DEPTH_TEXTURE,
		0, outHierarchicalDepthTexture, sampler);
}

void HierarchicalDepthBufferPass::generateHierarchicalDepthBuffer(vk::CommandBuffer cmdbuf)
{
	m_pipeline->bind(cmdbuf);
	m_pipeline->bindDescriptorSets(cmdbuf,
		{
			{.set = HIERARCHICALDEPTH_SET, .bindIdx = 0},
		});
	m_pipeline->updateDescriptorSets();
	outHierarchicalDepthTexture->transitionImageLayout(cmdbuf, vk::ImageLayout::eGeneral);
	glm::uvec2 currentMipDim(outHierarchicalDepthTexture->width,
		outHierarchicalDepthTexture->height);
	auto prevMipDim = currentMipDim;
	for (int i = 0; i < outHierarchicalDepthTexture->miplevels; i++)
	{
		HierarchicalDepthPushConst pushConst{
			.currentMipDimensions = currentMipDim,
			.prevMipDimensions = prevMipDim,
			.mipLevelIndex = i,
		};

		if (i != 0)
		{
			prevMipDim /= 2;
			prevMipDim.x = std::max(prevMipDim.x, 1u);
			prevMipDim.y = std::max(prevMipDim.y, 1u);
		}
		currentMipDim /= 2;
		currentMipDim.x = std::max(currentMipDim.x, 1u);
		currentMipDim.y = std::max(currentMipDim.y, 1u);
		
		m_pipeline->updatePushConstant(cmdbuf, vk::ShaderStageFlagBits::eCompute,
			sizeof(HierarchicalDepthPushConst), &pushConst);

		cmdbuf.dispatch(pushConst.currentMipDimensions.x / 16 + 1,
			pushConst.currentMipDimensions.y / 16 + 1, 1);

		if (i != outHierarchicalDepthTexture->miplevels - 1)
		{
			vk::ImageSubresourceRange range;
			range.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setBaseMipLevel(uint32_t(i))
				.setLayerCount(1)
				.setBaseArrayLayer(0)
				.setLevelCount(1);
			vk::ImageMemoryBarrier barrier;
			barrier.setSrcAccessMask(vk::AccessFlagBits::eShaderWrite)
				.setDstAccessMask(vk::AccessFlagBits::eShaderRead)
				.setOldLayout(vk::ImageLayout::eGeneral)
				.setNewLayout(vk::ImageLayout::eGeneral)
				.setImage(outHierarchicalDepthTexture->image)
				.setSubresourceRange(range);
			cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader,
				{}, {}, {}, barrier);
		}
	}
	outHierarchicalDepthTexture->transitionImageLayout(cmdbuf, vk::ImageLayout::eShaderReadOnlyOptimal);
}
