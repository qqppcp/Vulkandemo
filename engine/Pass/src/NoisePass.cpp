#include "NoisePass.h"
#include "define.h"
#include "Pipeline.h"
#include "Buffer.h"
#include "Texture.h"
#include "program.h"
#include <fstream>

constexpr uint32_t NOISE_SET = 0;
constexpr uint32_t BINDING_OUT_NOISE_TEXTURE = 0;
constexpr uint32_t BINDING_SOBOL_BUFFER = 1;
constexpr uint32_t BINDING_RANKING_TILE_BUFFER = 2;
constexpr uint32_t BINDING_SCRAMBLING_TILE_BUFFER = 3;

static int sobol_256spp_256d[256 * 256];
static int rankingTile[128 * 128 * 8] = { 0 };
static int scramblingTile[128 * 128 * 8];

struct NoisePushConst {
	uint32_t frameIndex;
};


NoisePass::NoisePass()
{
}

void NoisePass::init()
{
	outNoiseTexture = TextureManager::Instance().Create(128u, 128u, vk::Format::eR8G8Unorm,
		vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage);
	outNoiseTexture->layout = vk::ImageLayout::eUndefined;
	std::ifstream input;
	input.open(shaderPath + "sobol256spp256d.txt", std::ios::in);
	for (int i = 0; i < 256 * 256; i++)
	{
		input >> sobol_256spp_256d[i];
	}
	input.close();
	input.open(shaderPath + "scramblingTile.txt", std::ios::in);
	for (int i = 0; i < 128 * 128 * 8; i++)
	{
		input >> scramblingTile[i];
	}
	input.close();
	sobolBuffer.reset(new Buffer(sizeof(sobol_256spp_256d), vk::BufferUsageFlagBits::eTransferDst |
		vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal));
	rankingTileBuffer.reset(new Buffer(sizeof(rankingTile), vk::BufferUsageFlagBits::eTransferDst |
		vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal));
	scramblingTileBuffer.reset(new Buffer(sizeof(scramblingTile), vk::BufferUsageFlagBits::eTransferDst |
		vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal));
	UploadBufferData({}, sobolBuffer, sizeof(sobol_256spp_256d), sobol_256spp_256d);
	UploadBufferData({}, rankingTileBuffer, sizeof(rankingTile), rankingTile);
	UploadBufferData({}, scramblingTileBuffer, sizeof(scramblingTile), scramblingTile);
	std::shared_ptr<GPUProgram> shader = std::make_shared<GPUProgram>(shaderPath + "noisegen.comp.spv");
	std::vector<Pipeline::SetDescriptor> setLayouts;
	{
		Pipeline::SetDescriptor set;
		set.set = NOISE_SET;
		vk::DescriptorSetLayoutBinding binding;
		binding.setBinding(BINDING_OUT_NOISE_TEXTURE)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eStorageImage)
			.setStageFlags(vk::ShaderStageFlagBits::eCompute);
		set.bindings.push_back(binding);
		binding.setBinding(BINDING_SOBOL_BUFFER)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setStageFlags(vk::ShaderStageFlagBits::eCompute);
		set.bindings.push_back(binding);
		binding.setBinding(BINDING_RANKING_TILE_BUFFER)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setStageFlags(vk::ShaderStageFlagBits::eCompute);
		set.bindings.push_back(binding);
		binding.setBinding(BINDING_SCRAMBLING_TILE_BUFFER)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setStageFlags(vk::ShaderStageFlagBits::eCompute);
		set.bindings.push_back(binding);
		setLayouts.push_back(set);
	}
	std::vector<vk::PushConstantRange> range(1);
	range[0].setOffset(0)
		.setSize(sizeof(NoisePushConst))
		.setStageFlags(vk::ShaderStageFlagBits::eCompute);

	const Pipeline::ComputePipelineDescriptor desc = {
		.sets = setLayouts,
		.computerShader = shader->Compute,
		.pushConstants = range,
	};
	m_pipeline.reset(new Pipeline(desc));
	m_pipeline->allocateDescriptors({
		{.set = NOISE_SET, .count = 1},
		});
	m_pipeline->bindResource(NOISE_SET, BINDING_OUT_NOISE_TEXTURE, 0, outNoiseTexture, vk::DescriptorType::eStorageImage);
	m_pipeline->bindResource(NOISE_SET, BINDING_SOBOL_BUFFER, 0, sobolBuffer, 0, sobolBuffer->size, vk::DescriptorType::eStorageBuffer);
	m_pipeline->bindResource(NOISE_SET, BINDING_RANKING_TILE_BUFFER, 0, rankingTileBuffer, 0, rankingTileBuffer->size, vk::DescriptorType::eStorageBuffer);
	m_pipeline->bindResource(NOISE_SET, BINDING_SCRAMBLING_TILE_BUFFER, 0, scramblingTileBuffer, 0, scramblingTileBuffer->size, vk::DescriptorType::eStorageBuffer);
}

void NoisePass::generateNoise(vk::CommandBuffer cmdbuf)
{
	if (index == std::numeric_limits<uint32_t>::max())
	{
		index = 0;
	}

	NoisePushConst pushConst{
		.frameIndex = index,
	};
	
	m_pipeline->bind(cmdbuf);
	m_pipeline->updatePushConstant(cmdbuf, vk::ShaderStageFlagBits::eCompute, sizeof(NoisePushConst),
		&pushConst);
	m_pipeline->bindDescriptorSets(cmdbuf, {
		{.set = NOISE_SET, .bindIdx = 0},
		});
	m_pipeline->updateDescriptorSets();
	outNoiseTexture->transitionImageLayout(cmdbuf, vk::ImageLayout::eGeneral);
	cmdbuf.dispatch(128 / 16 + 1, 128 / 16 + 1, 1);
	outNoiseTexture->transitionImageLayout(cmdbuf, vk::ImageLayout::eShaderReadOnlyOptimal);
	index++;
}
