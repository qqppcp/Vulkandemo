#include "SkyboxPass.h"
#include "render_process.h"
#include "geometry.h"
#include "Pipeline.h"
#include "Context.h"
#include "program.h"
#include "Sampler.h"
#include "Texture.h"
#include "camera.h"
#include "define.h"
#include "mesh.h"


constexpr uint32_t TEXTURES_AND_SAMPLER_SET = 0U;
constexpr uint32_t VERTEX_INDEX_SET = 1U;

constexpr uint32_t BINDING_0 = 0U;

SkyboxPass::~SkyboxPass()
{
	Context::GetInstance().device.destroyFramebuffer(framebuffer);
	TextureManager::Instance().Destroy(hdrCubeTexture);
}

void SkyboxPass::init(std::shared_ptr<Texture> colorTexture, std::shared_ptr<Texture> depthTexture)
{
	colorTexture_ = colorTexture;
	depthTexture_ = depthTexture;
	hdrCubeTexture = TextureManager::Instance().LoadHDRCubemap(texturePath + "skybox.hdr", vk::Format::eB10G11R11UfloatPack32);
	width = colorTexture->width;
	height = colorTexture->height;
	{
		sampler.reset(new Sampler(vk::Filter::eLinear, vk::Filter::eLinear,
			vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
			vk::SamplerAddressMode::eRepeat, 10.0f));
	}
	{
		m_renderPass.reset(new RenderPass(std::vector<vk::Format>{colorTexture->format, depthTexture->format},
			std::vector<vk::ImageLayout>{vk::ImageLayout::eUndefined, vk::ImageLayout::eUndefined},
			std::vector<vk::ImageLayout>{vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eDepthStencilAttachmentOptimal },
			std::vector<vk::AttachmentLoadOp>{vk::AttachmentLoadOp::eDontCare, vk::AttachmentLoadOp::eDontCare},
			std::vector<vk::AttachmentStoreOp>{vk::AttachmentStoreOp::eStore, vk::AttachmentStoreOp::eStore},
			vk::PipelineBindPoint::eGraphics, {}, 1));
	}
	{
		std::vector<vk::ImageView> attachments{ colorTexture->view, depthTexture->view };
		vk::FramebufferCreateInfo framebufferCI;
		framebufferCI.setAttachments(attachments)
			.setLayers(1)
			.setRenderPass(m_renderPass->vkRenderPass())
			.setWidth(width)
			.setHeight(height);
		framebuffer = Context::GetInstance().device.createFramebuffer(framebufferCI);
	}
	{
		skyboxShader.reset(new GPUProgram(shaderPath + "skybox.vert.spv", shaderPath + "skybox.frag.spv"));
		auto mesh = GeometryManager::GetInstance().getMesh("cube");
		vertexBuffer.reset(new Buffer(mesh->vertices.size() * sizeof(Vertex), vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer,
			vk::MemoryPropertyFlagBits::eDeviceLocal));
		UploadBufferData({}, vertexBuffer, mesh->vertices.size() * sizeof(Vertex), mesh->vertices.data());
		std::vector<Pipeline::SetDescriptor> setLayouts;
		{
			Pipeline::SetDescriptor set;
			set.set = TEXTURES_AND_SAMPLER_SET;
			vk::DescriptorSetLayoutBinding binding;
			binding.setBinding(0)
				.setDescriptorCount(50)
				.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
				.setStageFlags(vk::ShaderStageFlagBits::eFragment);
			set.bindings.push_back(binding);
			setLayouts.push_back(set);
		}
		{
			Pipeline::SetDescriptor set;
			set.set = VERTEX_INDEX_SET;
			vk::DescriptorSetLayoutBinding binding;
			binding.setBinding(0)
				.setDescriptorCount(1)
				.setDescriptorType(vk::DescriptorType::eStorageBuffer)
				.setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
			set.bindings.push_back(binding);
			setLayouts.push_back(set);
		}
		std::vector<vk::PushConstantRange> ranges(1);
		ranges[0].setOffset(0)
			.setSize(sizeof(glm::mat4) * 2)
			.setStageFlags(vk::ShaderStageFlagBits::eVertex);
		const Pipeline::GraphicsPipelineDescriptor gpDesc = {
			.sets = setLayouts,
			.vertexShader = skyboxShader->Vertex,
			.fragmentShader = skyboxShader->Fragment,
			.pushConstants = { ranges },
			.dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor },
			.colorTextureFormats = { colorTexture->format },
			.depthTextureFormat = depthTexture->format,
			.cullMode = vk::CullModeFlagBits::eNone,
			.frontFace = vk::FrontFace::eClockwise,
			.viewport = vk::Viewport {0, 0, 0, 0},
			.depthTestEnable = true,
			.depthWriteEnable = true,
			.depthCompareOperation = vk::CompareOp::eLessOrEqual,
		};
		m_pipeline.reset(new Pipeline(gpDesc, m_renderPass->vkRenderPass()));
		m_pipeline->allocateDescriptors({
			{.set = TEXTURES_AND_SAMPLER_SET, .count = 1},
			{.set = VERTEX_INDEX_SET, .count = 1},
			});
		m_pipeline->bindResource(TEXTURES_AND_SAMPLER_SET, BINDING_0, 0, hdrCubeTexture, sampler);
		m_pipeline->bindResource(VERTEX_INDEX_SET, BINDING_0, 0, vertexBuffer, 0, vertexBuffer->size, vk::DescriptorType::eStorageBuffer);
	}
}

void SkyboxPass::render(vk::CommandBuffer cmdbuf, uint32_t index)
{
	std::array<glm::mat4, 2> c;
	c[0] = CameraManager::mainCamera->GetViewMatrix();
	c[1] = glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 1000.0f);

	std::vector<vk::ClearValue> clears(2);
	clears[0].setColor({ 0.5f, 0.3f, 0.6f, 1.0f });
	clears[1].setDepthStencil(1.0f);

	vk::RenderPassBeginInfo renderPassBI;
	renderPassBI.setRenderPass(m_renderPass->vkRenderPass())
		.setClearValues(clears)
		.setFramebuffer(framebuffer)
		.setRenderArea(VkRect2D({ 0,0 }, { width, height }));
	cmdbuf.beginRenderPass(renderPassBI, vk::SubpassContents::eInline);
	{
		cmdbuf.setViewport(0, { vk::Viewport{ 0, (float)0, (float)width, (float)height, 0.0f, 1.0f} });
		cmdbuf.setScissor(0, { vk::Rect2D{vk::Offset2D{0, 0}, vk::Extent2D{ width, height } } });
		m_pipeline->bind(cmdbuf);
		m_pipeline->bindDescriptorSets(cmdbuf, {
			{.set = TEXTURES_AND_SAMPLER_SET, .bindIdx = 0},
			{.set = VERTEX_INDEX_SET, .bindIdx = 0},
			});
		m_pipeline->updatePushConstant(cmdbuf, vk::ShaderStageFlagBits::eVertex, sizeof(glm::mat4) * 2, c.data());
		cmdbuf.draw(36, 1, 0, 0);
	}
	cmdbuf.endRenderPass();
}
