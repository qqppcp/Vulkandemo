#include "skyboxLayer.h"
#include "mesh.h"
#include "Sampler.h"
#include "render_process.h"
#include "Pipeline.h"
#include "program.h"
#include "Buffer.h"
#include "define.h"
#include "Context.h"
#include "window.h"
#include "camera.h"
#include "geometry.h"

#include <glm/gtc/type_ptr.hpp>

namespace
{
	std::vector<vk::Framebuffer> framebuffers;
}

SkyboxLayer::SkyboxLayer(std::string_view path)
{
	hdrT = TextureManager::Instance().LoadHDRCubemap(path.data(), vk::Format::eB10G11R11UfloatPack32);
	skybox.reset(new GPUProgram(shaderPath + "skybox.vert.spv", shaderPath + "skybox.frag.spv"));
	mesh = GeometryManager::GetInstance().getMesh("cube");
	vertexBuffer.reset(new Buffer(mesh->vertices.size() * sizeof(Vertex), vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal));
	UploadBufferData({}, vertexBuffer, mesh->vertices.size() * sizeof(Vertex), mesh->vertices.data());
	{
		sampler.reset(new Sampler(vk::Filter::eLinear, vk::Filter::eLinear,
			vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
			vk::SamplerAddressMode::eRepeat, 10.0f));
	}
	{
		renderPass.reset(new RenderPass(std::vector<vk::Format>{Context::GetInstance().swapchain->info.surfaceFormat.format, vk::Format::eD24UnormS8Uint},
			std::vector<vk::ImageLayout>{vk::ImageLayout::eUndefined, vk::ImageLayout::eUndefined},
			std::vector<vk::ImageLayout>{vk::ImageLayout::ePresentSrcKHR, vk::ImageLayout::eDepthStencilAttachmentOptimal },
			std::vector<vk::AttachmentLoadOp>{vk::AttachmentLoadOp::eDontCare, vk::AttachmentLoadOp::eDontCare},
			std::vector<vk::AttachmentStoreOp>{vk::AttachmentStoreOp::eStore, vk::AttachmentStoreOp::eDontCare},
			vk::PipelineBindPoint::eGraphics, {}, 1));
	}
	{
		for (int i = 0; i < Context::GetInstance().swapchain->imageviews.size(); i++)
		{
			auto [width, height] = GetWindowSize();
			std::vector<vk::ImageView> attachments{ Context::GetInstance().swapchain->imageviews[i], Context::GetInstance().depth->view };
			vk::FramebufferCreateInfo framebufferCI;
			framebufferCI.setAttachments(attachments)
				.setLayers(1)
				.setRenderPass(renderPass->vkRenderPass())
				.setWidth(width)
				.setHeight(height);
			framebuffers.emplace_back(Context::GetInstance().device.createFramebuffer(framebufferCI));
		}
	}
	{
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
			.vertexShader = skybox->Vertex,
			.fragmentShader = skybox->Fragment,
			.pushConstants = { ranges },
			.dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor,
				vk::DynamicState::eDepthTestEnable},
			.colorTextureFormats = {Context::GetInstance().swapchain->info.surfaceFormat.format},
			.depthTextureFormat = vk::Format::eD24UnormS8Uint,
			.cullMode = vk::CullModeFlagBits::eNone,
			.frontFace = vk::FrontFace::eClockwise,
			.viewport = vk::Viewport {0, 0, 0, 0},
			.blendEnable = true,
			.depthTestEnable = true,
			.depthWriteEnable = true,
			.depthCompareOperation = vk::CompareOp::eLessOrEqual,
		};
		pipeline.reset(new Pipeline(gpDesc, renderPass->vkRenderPass()));
		pipeline->allocateDescriptors({
			{.set = TEXTURES_AND_SAMPLER_SET, .count = 1},
			{.set = VERTEX_INDEX_SET, .count = 1},
			});
		pipeline->bindResource(TEXTURES_AND_SAMPLER_SET, 0, 0, hdrT, sampler);
		pipeline->bindResource(VERTEX_INDEX_SET, 0, 0, vertexBuffer, 0, vertexBuffer->size, vk::DescriptorType::eStorageBuffer);
	}
}

SkyboxLayer::~SkyboxLayer()
{
	for (auto framebuffer : framebuffers)
	{
		Context::GetInstance().device.destroyFramebuffer(framebuffer);
	}
	mesh.reset();
	skybox.reset();
	vertexBuffer.reset();
	sampler.reset();
	renderPass.reset();
	pipeline.reset();
	TextureManager::Instance().Destroy(hdrT);
}

void SkyboxLayer::OnRender()
{
	auto current_frame = Context::GetInstance().current_frame;
	auto& cmdbufs = Context::GetInstance().cmdbufs;
	auto [width, height] = GetWindowSize();
	std::array<glm::mat4, 2> c;
	c[0] = CameraManager::mainCamera->GetViewMatrix();
	c[1] = glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 1000.0f);
	vk::RenderPassBeginInfo renderPassBI;
	renderPassBI.setRenderPass(renderPass->vkRenderPass())
		.setFramebuffer(framebuffers[Context::GetInstance().image_index])
		.setRenderArea(VkRect2D({ 0,0 }, { width, height }));
	cmdbufs[current_frame].beginRenderPass(renderPassBI, vk::SubpassContents::eInline);
	{
		cmdbufs[current_frame].setViewport(0, { vk::Viewport{ 0, 0, (float)width, (float)height, 0.0f, 1.0f } });
		cmdbufs[current_frame].setScissor(0, { vk::Rect2D{vk::Offset2D{0, 0}, Context::GetInstance().swapchain->info.imageExtent} });
		cmdbufs[current_frame].setDepthTestEnable(VK_TRUE);
		pipeline->bind(cmdbufs[current_frame]);
		pipeline->bindDescriptorSets(cmdbufs[current_frame],
			{ 
			{.set = TEXTURES_AND_SAMPLER_SET, .bindIdx = 0},
			{.set = VERTEX_INDEX_SET, .bindIdx = 0},
			 });

		cmdbufs[current_frame].pushConstants(pipeline->vkPipelineLayout(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4) * 2, c.data());
		cmdbufs[current_frame].draw(mesh->vertices.size(), 1, 0, 0);
	}
	cmdbufs[current_frame].endRenderPass();
}
