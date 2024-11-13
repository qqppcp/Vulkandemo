#include "LightingPass.h"
#include "render_process.h"
#include "Pipeline.h"
#include "Texture.h"
#include "Buffer.h"
#include "Sampler.h"
#include "program.h"
#include "Context.h"
#include "define.h"

constexpr uint32_t GBUFFERDATA_SET = 0;

constexpr uint32_t BINDING_WORLDNORMAL = 0;
constexpr uint32_t BINDING_SPECULAR = 1;
constexpr uint32_t BINDING_BASECOLOR = 2;
constexpr uint32_t BINDING_DEPTH = 3;
constexpr uint32_t BINDING_POSITION = 4;
constexpr uint32_t BINDING_AMBIENTOCCLUSION = 5;
constexpr uint32_t BINDING_SHADOWDEPTH = 6;

constexpr uint32_t TRANSFORM_LIGHT_DATA_SET = 1;
constexpr uint32_t BINDING_TRANSFORM = 0;
constexpr uint32_t BINDING_LIGHT = 1;

struct Transforms 
{
	glm::aligned_mat4 viewProj;
	glm::aligned_mat4 viewProjInv;
	glm::aligned_mat4 viewInv;
};

LightingPass::LightingPass() {}

LightingPass::~LightingPass()
{
	Context::GetInstance().device.unmapMemory(stageBuffer1->memory);
	Context::GetInstance().device.unmapMemory(stageBuffer2->memory);
	Context::GetInstance().device.destroyFramebuffer(frameBuffer);
}

void LightingPass::init(std::shared_ptr<Texture> gBufferNormal, 
						std::shared_ptr<Texture> gBufferSpecular, 
						std::shared_ptr<Texture> gBufferBaseColor, 
						std::shared_ptr<Texture> gBufferPosition, 
						std::shared_ptr<Texture> gBufferDepth, 
						std::shared_ptr<Texture> ambientOcclusion, 
						std::shared_ptr<Texture> shadowDepth)
{
	width = Context::GetInstance().swapchain->info.imageExtent.width;
	height = Context::GetInstance().swapchain->info.imageExtent.height;
	this->gBufferNormal = gBufferNormal;
	this->gBufferSpecular = gBufferSpecular;
	this->gBufferBaseColor = gBufferBaseColor;
	this->gBufferPosition = gBufferPosition;
	this->gBufferDepth = gBufferDepth;
	this->ambientOcclusion = ambientOcclusion;
	this->shadowDepth = shadowDepth;
	sampler.reset(new Sampler(vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerAddressMode::eClampToEdge,
		vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge, 100.0f));
	samplerShadowMap.reset(new Sampler(vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerAddressMode::eClampToEdge,
		vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge, 1.0f, true, vk::CompareOp::eLess));
	outLightingTexture = TextureManager::Instance().Create(width, height, vk::Format::eB8G8R8A8Unorm,
		vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
		vk::ImageUsageFlagBits::eStorage);
	cameraBuffer.reset(new Buffer(sizeof(Transforms), vk::BufferUsageFlagBits::eUniformBuffer|
		vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal));
	stageBuffer1.reset(new Buffer(sizeof(Transforms), vk::BufferUsageFlagBits::eTransferSrc
		, vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible));
	lightBuffer.reset(new Buffer(sizeof(LightData), vk::BufferUsageFlagBits::eUniformBuffer |
		vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal));
	stageBuffer2.reset(new Buffer(sizeof(LightData), vk::BufferUsageFlagBits::eTransferSrc
		, vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible));
	ptr1 = Context::GetInstance().device.mapMemory(stageBuffer1->memory, 0, sizeof(Transforms));
	ptr2 = Context::GetInstance().device.mapMemory(stageBuffer2->memory, 0, sizeof(Transforms));
	m_renderPass.reset(new RenderPass(std::vector<vk::Format>{vk::Format::eB8G8R8A8Unorm},
		std::vector<vk::ImageLayout>{vk::ImageLayout::eUndefined},
		std::vector<vk::ImageLayout>{vk::ImageLayout::eShaderReadOnlyOptimal},
		std::vector<vk::AttachmentLoadOp>{vk::AttachmentLoadOp::eClear},
		std::vector<vk::AttachmentStoreOp>{vk::AttachmentStoreOp::eStore},
		vk::PipelineBindPoint::eGraphics, {}, 1));

	std::vector<vk::ImageView> attachments{ outLightingTexture->view };
	vk::FramebufferCreateInfo createInfo;
	createInfo.setAttachments(attachments)
		.setHeight(height)
		.setWidth(width)
		.setRenderPass(m_renderPass->vkRenderPass())
		.setLayers(1);
	frameBuffer = Context::GetInstance().device.createFramebuffer(createInfo);
	auto shader = std::make_shared<GPUProgram>(shaderPath + "fullscreen.vert.spv", shaderPath + "lighting.frag.spv");
	std::vector<Pipeline::SetDescriptor> setLayouts;
	{
		Pipeline::SetDescriptor set;
		set.set = GBUFFERDATA_SET;
		vk::DescriptorSetLayoutBinding binding;
		binding.setBinding(BINDING_WORLDNORMAL)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setStageFlags(vk::ShaderStageFlagBits::eFragment);
		set.bindings.push_back(binding);
		binding.setBinding(BINDING_SPECULAR)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setStageFlags(vk::ShaderStageFlagBits::eFragment);
		set.bindings.push_back(binding);
		binding.setBinding(BINDING_BASECOLOR)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setStageFlags(vk::ShaderStageFlagBits::eFragment);
		set.bindings.push_back(binding);
		binding.setBinding(BINDING_DEPTH)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setStageFlags(vk::ShaderStageFlagBits::eFragment);
		set.bindings.push_back(binding);
		binding.setBinding(BINDING_POSITION)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setStageFlags(vk::ShaderStageFlagBits::eFragment);
		set.bindings.push_back(binding);
		binding.setBinding(BINDING_AMBIENTOCCLUSION)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setStageFlags(vk::ShaderStageFlagBits::eFragment);
		set.bindings.push_back(binding);
		binding.setBinding(BINDING_SHADOWDEPTH)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setStageFlags(vk::ShaderStageFlagBits::eFragment);
		set.bindings.push_back(binding);
		setLayouts.push_back(set);
	}
	{
		Pipeline::SetDescriptor set;
		set.set = TRANSFORM_LIGHT_DATA_SET;
		vk::DescriptorSetLayoutBinding binding;
		binding.setBinding(BINDING_TRANSFORM)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setStageFlags(vk::ShaderStageFlagBits::eFragment);
		set.bindings.push_back(binding);
		binding.setBinding(BINDING_LIGHT)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setStageFlags(vk::ShaderStageFlagBits::eFragment);
		set.bindings.push_back(binding);
		setLayouts.push_back(set);
	}
	const Pipeline::GraphicsPipelineDescriptor gpDesc = {
		.sets = setLayouts,
		.vertexShader = shader->Vertex,
		.fragmentShader = shader->Fragment,
		.dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor },
		.colorTextureFormats = {vk::Format::eB8G8R8A8Unorm},
		.primitiveTopology = vk::PrimitiveTopology::eTriangleStrip,
		.sampleCount = vk::SampleCountFlagBits::e1,
		.cullMode = vk::CullModeFlagBits::eNone,
		.viewport = {0,0,0,0},
		.depthTestEnable = false,
		.depthWriteEnable = false,
		.depthCompareOperation = vk::CompareOp::eAlways,
	};
	m_pipeline.reset(new Pipeline(gpDesc, m_renderPass->vkRenderPass()));

	m_pipeline->allocateDescriptors({
		{.set = GBUFFERDATA_SET, .count = 1},
		{.set = TRANSFORM_LIGHT_DATA_SET, .count = 1},
		});
	m_pipeline->bindResource(GBUFFERDATA_SET, BINDING_WORLDNORMAL, 0, gBufferNormal, sampler);
	m_pipeline->bindResource(GBUFFERDATA_SET, BINDING_SPECULAR, 0, gBufferSpecular, sampler);
	m_pipeline->bindResource(GBUFFERDATA_SET, BINDING_BASECOLOR, 0, gBufferBaseColor, sampler);
	m_pipeline->bindResource(GBUFFERDATA_SET, BINDING_DEPTH, 0, gBufferDepth, sampler);
	m_pipeline->bindResource(GBUFFERDATA_SET, BINDING_POSITION, 0, gBufferPosition, sampler);
	m_pipeline->bindResource(GBUFFERDATA_SET, BINDING_AMBIENTOCCLUSION, 0, ambientOcclusion, sampler);
	
	// enable this to use sampler2d instead of sampler2dShadow, also add #define
	// USESAMPLERFORSHADOW line in lighting.frag shader
	const bool useSampler2DForShadows = false;
	m_pipeline->bindResource(GBUFFERDATA_SET, BINDING_SHADOWDEPTH, 0, shadowDepth,
		useSampler2DForShadows ? sampler : samplerShadowMap);
	m_pipeline->bindResource(TRANSFORM_LIGHT_DATA_SET, BINDING_TRANSFORM, 0, cameraBuffer,
		0, sizeof(Transforms), vk::DescriptorType::eUniformBuffer);
	m_pipeline->bindResource(TRANSFORM_LIGHT_DATA_SET, BINDING_LIGHT, 0, lightBuffer, 0,
		sizeof(LightData), vk::DescriptorType::eUniformBuffer);
}

void LightingPass::render(vk::CommandBuffer cmdbuf, uint32_t index, const LightData& data, const glm::mat4& viewMat, const glm::mat4& projMat)
{
	glm::mat4 viewProjMat = projMat * viewMat;
	Transforms transform;
	transform.viewProj = viewProjMat;
	transform.viewProjInv = glm::inverse(viewProjMat);
	transform.viewInv = glm::inverse(viewMat);
	memcpy(ptr1, &transform, sizeof(Transforms));
	memcpy(ptr2, &data, sizeof(LightData));
	CopyBuffer(stageBuffer1->buffer, cameraBuffer->buffer, sizeof(Transforms), 0, 0);
	CopyBuffer(stageBuffer2->buffer, lightBuffer->buffer, sizeof(LightData), 0, 0);
	
	std::array<vk::ClearValue, 1> clearValues;
	clearValues[0].setColor({ 0.f, 1.f, 0.f, 0.f });
	vk::RenderPassBeginInfo beginInfo;
	beginInfo.setRenderPass(m_renderPass->vkRenderPass())
		.setFramebuffer(frameBuffer)
		.setClearValues(clearValues)
		.setRenderArea({ vk::Offset2D{0,0}, vk::Extent2D{width, height} });
	
	cmdbuf.beginRenderPass(beginInfo, vk::SubpassContents::eInline);
	{
		cmdbuf.setViewport(0, { vk::Viewport{ 0, (float)height, (float)width, -(float)height, 0.0f, 1.0f} });
		cmdbuf.setScissor(0, { vk::Rect2D{vk::Offset2D{0, 0}, vk::Extent2D{width, height}} });
		m_pipeline->bind(cmdbuf);
		m_pipeline->bindDescriptorSets(cmdbuf, {
			{.set = GBUFFERDATA_SET, .bindIdx = 0},
			{.set = TRANSFORM_LIGHT_DATA_SET, .bindIdx = 0},
			});
		m_pipeline->updateDescriptorSets();
		cmdbuf.draw(4, 1, 0, 0);
	}
	cmdbuf.endRenderPass();
	outLightingTexture->layout = vk::ImageLayout::eShaderReadOnlyOptimal;
}
