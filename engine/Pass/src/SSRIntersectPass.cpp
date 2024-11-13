#include "SSRIntersectPass.h"
#include "define.h"
#include "render_process.h"
#include "Pipeline.h"
#include "program.h"
#include "Texture.h"
#include "Buffer.h"
#include "Context.h"
#include "camera.h"
#include <glm/glm.hpp>
#include <glm/gtx/type_aligned.hpp>

constexpr uint32_t SSR_INTERSECT_OUTPUT_SET = 0;
constexpr uint32_t BINDING_OUT_SSR_INTERSECT = 0;

constexpr uint32_t INPUT_TEXTURES_SET = 1;
constexpr uint32_t BINDING_GBUFFER_WORLDNORMAL = 0;
constexpr uint32_t BINDING_GBUFFER_SPECULAR = 1;
constexpr uint32_t BINDING_GBUFFER_BASECOLOR = 2;
constexpr uint32_t BINDING_HIERARCHICALDEPTH = 3;
constexpr uint32_t BINDING_NOISE = 4;

constexpr uint32_t INPUT_CAMERA_SET = 2;
constexpr uint32_t BINDING_CAMERA_TRANSFORM = 0;

struct Transforms {
	glm::aligned_mat4 model;
	glm::aligned_mat4 view;
	glm::aligned_mat4 projection;
	glm::aligned_mat4 projectionInv;
	glm::aligned_mat4 viewInv;
};

struct PushConst {
	glm::uvec2 resolution;
	uint32_t frameIndex;
};

SSRIntersectPass::SSRIntersectPass()
{
}

SSRIntersectPass::~SSRIntersectPass()
{
	Context::GetInstance().device.unmapMemory(stageBuffer->memory);
}

void SSRIntersectPass::init(std::shared_ptr<Texture> gBufferNormal, std::shared_ptr<Texture> gBufferSpecular, std::shared_ptr<Texture> gBufferBaseColor, std::shared_ptr<Texture> hierarchicalDepth, std::shared_ptr<Texture> noiseTexture)
{
	this->gBufferNormal = gBufferNormal;
	this->gBufferSpecular = gBufferSpecular;
	this->gBufferBaseColor = gBufferBaseColor;
	this->hierarchicalDepth = hierarchicalDepth;
	this->noiseTexture = noiseTexture;
	width = Context::GetInstance().swapchain->info.imageExtent.width;
	height = Context::GetInstance().swapchain->info.imageExtent.height;
	auto mipLevels = getMipLevelsCount(width, height);

	sampler.reset(new Sampler(vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerAddressMode::eClampToEdge,
		vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge, 100.0f));

	outSSRIntersectTexture = TextureManager::Instance().Create(width, height, vk::Format::eR16G16B16A16Sfloat,
		vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage, mipLevels);
	outSSRIntersectTexture->layout = vk::ImageLayout::eUndefined;

	cameraBuffer.reset(new Buffer(sizeof(Transforms), vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst,
		vk::MemoryPropertyFlagBits::eDeviceLocal));
	stageBuffer.reset(new Buffer(sizeof(Transforms), vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible));
	ptr = Context::GetInstance().device.mapMemory(stageBuffer->memory, 0, sizeof(Transforms));
	auto shader = std::make_shared<GPUProgram>(shaderPath + "ssr.comp.spv");
	std::vector<Pipeline::SetDescriptor> setLayouts;
	{
		Pipeline::SetDescriptor set;
		set.set = SSR_INTERSECT_OUTPUT_SET;
		vk::DescriptorSetLayoutBinding binding;
		binding.setBinding(BINDING_OUT_SSR_INTERSECT)
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
		binding.setBinding(BINDING_GBUFFER_WORLDNORMAL)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setStageFlags(vk::ShaderStageFlagBits::eCompute);
		set.bindings.push_back(binding);
		binding.setBinding(BINDING_GBUFFER_SPECULAR)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setStageFlags(vk::ShaderStageFlagBits::eCompute);
		set.bindings.push_back(binding);
		binding.setBinding(BINDING_GBUFFER_BASECOLOR)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setStageFlags(vk::ShaderStageFlagBits::eCompute);
		set.bindings.push_back(binding);
		binding.setBinding(BINDING_HIERARCHICALDEPTH)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setStageFlags(vk::ShaderStageFlagBits::eCompute);
		set.bindings.push_back(binding);
		binding.setBinding(BINDING_NOISE)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setStageFlags(vk::ShaderStageFlagBits::eCompute);
		set.bindings.push_back(binding);
		setLayouts.push_back(set);
	}
	{
		Pipeline::SetDescriptor set;
		set.set = INPUT_CAMERA_SET;
		vk::DescriptorSetLayoutBinding binding;
		binding.setBinding(BINDING_CAMERA_TRANSFORM)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
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
		{.set = SSR_INTERSECT_OUTPUT_SET, .count = 1},
		{.set = INPUT_TEXTURES_SET, .count = 1},
		{.set = INPUT_CAMERA_SET, .count = 1},
		});
	m_pipeline->bindResource(SSR_INTERSECT_OUTPUT_SET, BINDING_OUT_SSR_INTERSECT, 0,
		outSSRIntersectTexture, vk::DescriptorType::eStorageImage);
	m_pipeline->bindResource(INPUT_TEXTURES_SET, BINDING_GBUFFER_WORLDNORMAL, 0,
		gBufferNormal, sampler);
	m_pipeline->bindResource(INPUT_TEXTURES_SET, BINDING_GBUFFER_SPECULAR, 0,
		gBufferSpecular, sampler);
	m_pipeline->bindResource(INPUT_TEXTURES_SET, BINDING_GBUFFER_BASECOLOR, 0,
		gBufferBaseColor, sampler);
	m_pipeline->bindResource(INPUT_TEXTURES_SET, BINDING_HIERARCHICALDEPTH, 0,
		hierarchicalDepth, sampler);
	m_pipeline->bindResource(INPUT_TEXTURES_SET, BINDING_NOISE, 0,
		noiseTexture, sampler);
	m_pipeline->bindResource(INPUT_CAMERA_SET, BINDING_CAMERA_TRANSFORM, 0,
		cameraBuffer, 0, sizeof(Transforms), vk::DescriptorType::eUniformBuffer);
}

void SSRIntersectPass::run(vk::CommandBuffer cmdbuf)
{
	if (index == std::numeric_limits<uint32_t>::max()) {
		index = 0;
	}
	Transforms transform;
	transform.model = glm::mat4(1.0f);
	transform.view = CameraManager::mainCamera->GetViewMatrix();
	transform.projection = glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 1000.0f);
	transform.viewInv = glm::inverse(transform.view);
	transform.projectionInv = glm::inverse(transform.projection);
	memcpy(ptr, &transform, sizeof(Transforms));
	CopyBuffer(stageBuffer->buffer, cameraBuffer->buffer, sizeof(Transforms), 0, 0);
	m_pipeline->bind(cmdbuf);
	PushConst pushConst{
		.resolution = glm::uvec2(width, height),
		.frameIndex = index,
	};
	m_pipeline->updatePushConstant(cmdbuf, vk::ShaderStageFlagBits::eCompute, sizeof(PushConst), &pushConst);
	m_pipeline->bindDescriptorSets(cmdbuf, {
		{.set = SSR_INTERSECT_OUTPUT_SET, .bindIdx = 0},
		{.set = INPUT_TEXTURES_SET, .bindIdx = 0},
		{.set = INPUT_CAMERA_SET, .bindIdx = 0},
		});
	m_pipeline->updateDescriptorSets();
	outSSRIntersectTexture->transitionImageLayout(cmdbuf, vk::ImageLayout::eGeneral);
	cmdbuf.dispatch(pushConst.resolution.x / 16 + 1, pushConst.resolution.y / 16 + 1, 1);
	outSSRIntersectTexture->transitionImageLayout(cmdbuf, vk::ImageLayout::eShaderReadOnlyOptimal);
	index++;
}
