#include "SSAOPass.h"
#include "Pipeline.h"
#include "Sampler.h"
#include "Context.h"
#include "program.h"
#include "define.h"
#include "Texture.h"
#include <glm/glm.hpp>

constexpr uint32_t SSAO_OUTPUT_SET = 0;
constexpr uint32_t BINDING_OUT_SSAO = 0;

constexpr uint32_t INPUT_TEXTURES_SET = 1;
constexpr uint32_t BINDING_GBUFFER_DEPTH = 0;

struct PushConst {
	glm::uvec2 resolution;
	uint32_t frameIndex;
};

SSAOPass::SSAOPass() {}

SSAOPass::~SSAOPass() {}

void SSAOPass::init(std::shared_ptr<Texture> gBufferDepth)
{
	this->gBufferDepth = gBufferDepth;
	auto width = Context::GetInstance().swapchain->info.imageExtent.width;
	auto height = Context::GetInstance().swapchain->info.imageExtent.height;
	{
		sampler.reset(new Sampler(vk::Filter::eLinear, vk::Filter::eLinear,
			vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge,
			vk::SamplerAddressMode::eClampToEdge, 100.0f));
	}
	{
		outSSAOTexture = TextureManager::Instance().Create(width, height, vk::Format::eR8G8B8A8Unorm,
			vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage, getMipLevelsCount(width, height));
		outSSAOTexture->layout = vk::ImageLayout::eUndefined;
	}
	std::shared_ptr<GPUProgram> shader = std::make_shared<GPUProgram>(shaderPath + "ssao.comp.spv");
	std::vector<Pipeline::SetDescriptor> setLayouts;
	{
		Pipeline::SetDescriptor set;
		set.set = SSAO_OUTPUT_SET;
		vk::DescriptorSetLayoutBinding binding;
		binding.setBinding(BINDING_OUT_SSAO)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eStorageImage)
			.setStageFlags(vk::ShaderStageFlagBits::eCompute);
		set.bindings.push_back(binding);
		setLayouts.push_back(set);
	}
	{
		Pipeline::SetDescriptor set;
		set.set = INPUT_TEXTURES_SET;
		vk::DescriptorSetLayoutBinding binding;
		binding.setBinding(BINDING_GBUFFER_DEPTH)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setStageFlags(vk::ShaderStageFlagBits::eCompute);
		set.bindings.push_back(binding);
		setLayouts.push_back(set);
	}
	std::vector<vk::PushConstantRange> range(1);
	range[0].setOffset(0)
		.setSize(sizeof(PushConst))
		.setStageFlags(vk::ShaderStageFlagBits::eCompute);

	const Pipeline::ComputePipelineDescriptor desc = {
		.sets = setLayouts,
		.computerShader = shader->Compute,
		.pushConstants = range,
	};
	m_pipeline.reset(new Pipeline(desc));
	m_pipeline->allocateDescriptors({
		{.set = SSAO_OUTPUT_SET, .count = 1},
		{.set = INPUT_TEXTURES_SET, .count = 1},
		});
	m_pipeline->bindResource(SSAO_OUTPUT_SET, BINDING_OUT_SSAO, 0, outSSAOTexture, vk::DescriptorType::eStorageImage);
	m_pipeline->bindResource(INPUT_TEXTURES_SET, BINDING_GBUFFER_DEPTH, 0, gBufferDepth, sampler);
}

void SSAOPass::run(vk::CommandBuffer cmdbuf)
{
	m_pipeline->bind(cmdbuf);
	PushConst pushConst{
		.resolution = glm::uvec2(Context::GetInstance().swapchain->info.imageExtent.width,
		Context::GetInstance().swapchain->info.imageExtent.height),
		.frameIndex = 0,
	};

	m_pipeline->updatePushConstant(cmdbuf, vk::ShaderStageFlagBits::eCompute, sizeof(PushConst), &pushConst);
	m_pipeline->bindDescriptorSets(cmdbuf, {
		{.set = SSAO_OUTPUT_SET, .bindIdx = 0},
		{.set = INPUT_TEXTURES_SET, .bindIdx = 0},
		});
	m_pipeline->updateDescriptorSets();
	outSSAOTexture->transitionImageLayout(cmdbuf, vk::ImageLayout::eGeneral);
	cmdbuf.dispatch(pushConst.resolution.x / 16 + 1, pushConst.resolution.y / 16 + 1, 1);
	outSSAOTexture->transitionImageLayout(cmdbuf, vk::ImageLayout::eShaderReadOnlyOptimal);
}
