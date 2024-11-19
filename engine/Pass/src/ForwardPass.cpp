#include "ForwardPass.h"
#include "Pipeline.h"
#include "Context.h"
#include "program.h"
#include "render_process.h"
#include "Buffer.h"
#include "Sampler.h"
#include "Texture.h"
#include "camera.h"
#include "define.h"
#include "LightData.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

constexpr uint32_t CAMERA_SET = 0U;
constexpr uint32_t TEXTURES_SET = 1U;
constexpr uint32_t SAMPLER_SET = 2U;
constexpr uint32_t STORAGE_BUFFER_SET = 3U;
constexpr uint32_t LIGHT_SET = 4U;

struct Transforms
{
	glm::aligned_mat4 viewProj;
	glm::aligned_mat4 viewProjInv;
	glm::aligned_mat4 viewInv;
};

namespace
{
	LightData light;
	Transforms transform;
}

ForwardPass::~ForwardPass()
{
	for (auto framebuffer : framebuffers)
	{
		Context::GetInstance().device.destroyFramebuffer(framebuffer);
	}
	Context::GetInstance().device.unmapMemory(stageBuffer1->memory);
	Context::GetInstance().device.unmapMemory(stageBuffer2->memory);
}

void ForwardPass::init(std::vector<RenderTarget>& rts)
{
	width = rts[0].width;
	height = rts[0].height;
	{
		light.lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		light.lightPos = glm::vec4(0.0f, 1.f, 0.0f, 1.0f);
		cameraBuffer.reset(new Buffer(sizeof(Transforms), vk::BufferUsageFlagBits::eUniformBuffer |
			vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal));
		stageBuffer1.reset(new Buffer(sizeof(Transforms), vk::BufferUsageFlagBits::eTransferSrc
			, vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible));
		lightBuffer.reset(new Buffer(sizeof(LightData), vk::BufferUsageFlagBits::eUniformBuffer |
			vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal));
		stageBuffer2.reset(new Buffer(sizeof(LightData), vk::BufferUsageFlagBits::eTransferSrc
			, vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible));
		ptr1 = Context::GetInstance().device.mapMemory(stageBuffer1->memory, 0, sizeof(Transforms));
		ptr2 = Context::GetInstance().device.mapMemory(stageBuffer2->memory, 0, sizeof(Transforms));
	}
	{
		std::vector<vk::Format> formats = rts[0].formats;
		formats.push_back(rts[0].depthFormat);
		std::vector<vk::AttachmentLoadOp> loads;
		std::vector<vk::AttachmentStoreOp> stores;
		for (int i = 0; i < rts[0].clears.size(); i++)
		{
			switch (rts[0].clears[i])
			{
			case LoadOp::Clear:
				loads.push_back(vk::AttachmentLoadOp::eClear); break;
			case LoadOp::Load:
				loads.push_back(vk::AttachmentLoadOp::eLoad); break;
			default:
				loads.push_back(vk::AttachmentLoadOp::eDontCare);
				break;
			}
			stores.push_back(vk::AttachmentStoreOp::eStore);
		}
		m_renderPass.reset(new RenderPass(formats,
			std::vector<vk::ImageLayout>{vk::ImageLayout::eUndefined, vk::ImageLayout::eUndefined},
			std::vector<vk::ImageLayout>{vk::ImageLayout::ePresentSrcKHR, vk::ImageLayout::eDepthStencilAttachmentOptimal },
			loads,
			stores,
			vk::PipelineBindPoint::eGraphics, {}, 1));
	}
	{
		for (int i = 0; i < rts.size(); i++)
		{
			std::vector<vk::ImageView> attachments;
			for (int j = 0; j < rts[i].views.size(); j++)
			{
				attachments.push_back(rts[i].views[j]);
			}
			if (rts[i].depth != -1)
			{
				attachments.push_back(rts[i].depthView);
			}
			vk::FramebufferCreateInfo framebufferCI;
			framebufferCI.setAttachments(attachments)
				.setLayers(1)
				.setRenderPass(m_renderPass->vkRenderPass())
				.setWidth(width)
				.setHeight(height);
			framebuffers.emplace_back(Context::GetInstance().device.createFramebuffer(framebufferCI));
		}
	}
	{
		blinnShader.reset(new GPUProgram(shaderPath + "blinn-phong.vert.spv", shaderPath + "blinn-phong.frag.spv"));
		std::vector<Pipeline::SetDescriptor> setLayouts;
		{
			Pipeline::SetDescriptor set;
			set.set = CAMERA_SET;
			vk::DescriptorSetLayoutBinding binding;
			binding.setBinding(0)
				.setDescriptorCount(1)
				.setDescriptorType(vk::DescriptorType::eUniformBuffer)
				.setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
			set.bindings.push_back(binding);
			setLayouts.push_back(set);
		}
		{
			Pipeline::SetDescriptor set;
			set.set = TEXTURES_SET;
			vk::DescriptorSetLayoutBinding binding;
			binding.setBinding(0)
				.setDescriptorCount(500)
				.setDescriptorType(vk::DescriptorType::eSampledImage)
				.setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
			set.bindings.push_back(binding);
			setLayouts.push_back(set);
		}
		{
			Pipeline::SetDescriptor set;
			set.set = SAMPLER_SET;
			vk::DescriptorSetLayoutBinding binding;
			binding.setBinding(0)
				.setDescriptorCount(50)
				.setDescriptorType(vk::DescriptorType::eSampler)
				.setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
			set.bindings.push_back(binding);
			setLayouts.push_back(set);
		}
		{
			Pipeline::SetDescriptor set;
			set.set = STORAGE_BUFFER_SET;
			vk::DescriptorSetLayoutBinding binding;
			binding.setBinding(0)
				.setDescriptorCount(4)
				.setDescriptorType(vk::DescriptorType::eStorageBuffer)
				.setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
			set.bindings.push_back(binding);
			setLayouts.push_back(set);
		}
		{
			Pipeline::SetDescriptor set;
			set.set = LIGHT_SET;
			vk::DescriptorSetLayoutBinding binding;
			binding.setBinding(0)
				.setDescriptorCount(1)
				.setDescriptorType(vk::DescriptorType::eUniformBuffer)
				.setStageFlags(vk::ShaderStageFlagBits::eFragment);
			set.bindings.push_back(binding);
			binding.setBinding(1)
				.setDescriptorCount(1)
				.setDescriptorType(vk::DescriptorType::eUniformBuffer)
				.setStageFlags(vk::ShaderStageFlagBits::eFragment);
			set.bindings.push_back(binding);
			setLayouts.push_back(set);
		}
		const Pipeline::GraphicsPipelineDescriptor gpDesc = {
			.sets = setLayouts,
			.vertexShader = blinnShader->Vertex,
			.fragmentShader = blinnShader->Fragment,
			.dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor},
			.colorTextureFormats = {rts[0].formats[0]},
			.depthTextureFormat = rts[0].depthFormat,
			.cullMode = vk::CullModeFlagBits::eNone,
			.frontFace = vk::FrontFace::eClockwise,
			.viewport = vk::Viewport {0, 0, 0, 0},
			.blendEnable = true,
			.depthTestEnable = true,
			.depthWriteEnable = true,
			.depthCompareOperation = vk::CompareOp::eLess,
		};
		m_pipeline.reset(new Pipeline(gpDesc, m_renderPass->vkRenderPass()));
		m_pipeline->allocateDescriptors({
			{.set = CAMERA_SET, .count = 1},
			{.set = TEXTURES_SET, .count = 1},
			{.set = SAMPLER_SET, .count = 1},
			{.set = STORAGE_BUFFER_SET, .count = 1},
			{.set = LIGHT_SET, .count = 1},
			});
		m_pipeline->bindResource(LIGHT_SET, 0, 0, cameraBuffer,
			0, sizeof(Transforms), vk::DescriptorType::eUniformBuffer);
		m_pipeline->bindResource(LIGHT_SET, 1, 0, lightBuffer, 0,
			sizeof(LightData), vk::DescriptorType::eUniformBuffer);
	}
}

void ForwardPass::render(vk::CommandBuffer cmdbuf, uint32_t index, vk::Buffer indexBuffer,
	vk::Buffer indirectDrawBuffer, vk::Buffer indirectDrawCountBuffer,
	uint32_t numMeshes, uint32_t bufferSize)
{
	auto view = CameraManager::mainCamera->GetViewMatrix();
	auto project = glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 1000.0f);
	glm::mat4 viewProjMat = project * view;
	transform.viewProj = viewProjMat;
	transform.viewProjInv = glm::inverse(viewProjMat);
	transform.viewInv = glm::inverse(view);
	memcpy(ptr1, &transform, sizeof(Transforms));
	memcpy(ptr2, &light, sizeof(LightData));
	CopyBuffer(stageBuffer1->buffer, cameraBuffer->buffer, sizeof(Transforms), 0, 0);
	CopyBuffer(stageBuffer2->buffer, lightBuffer->buffer, sizeof(LightData), 0, 0);

	vk::RenderPassBeginInfo renderPassBI;

	std::array<vk::ClearValue, 2> clear;
	clear[0].setColor({ 0.05f, 0.05f, 0.05f, 1.0f });
	clear[1].setDepthStencil(1.0f);
	renderPassBI.setRenderPass(m_renderPass->vkRenderPass())
		.setFramebuffer(framebuffers[index])
		.setRenderArea(VkRect2D({ 0,0 }, { width, height }))
		.setClearValues(clear);
	cmdbuf.beginRenderPass(renderPassBI, vk::SubpassContents::eInline);
	{
		cmdbuf.setViewport(0, { vk::Viewport{ 0, (float)height, (float)width, -(float)height, 0.0f, 1.0f} });
		cmdbuf.setScissor(0, { vk::Rect2D{vk::Offset2D{0, 0}, vk::Extent2D{ width, height } } });
		m_pipeline->bind(cmdbuf);
		m_pipeline->bindDescriptorSets(cmdbuf, {
			{.set = CAMERA_SET, .bindIdx = 0},
			{.set = TEXTURES_SET, .bindIdx = 0},
			{.set = SAMPLER_SET, .bindIdx = 0},
			{.set = STORAGE_BUFFER_SET, .bindIdx = 0},
			{.set = LIGHT_SET, .bindIdx = 0},
			});
		m_pipeline->updateDescriptorSets();
		cmdbuf.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);
		cmdbuf.drawIndexedIndirectCount(indirectDrawBuffer, 0, indirectDrawCountBuffer,
			0, numMeshes, bufferSize);
	}
	cmdbuf.endRenderPass();
}

glm::vec3 ForwardPass::LightPos()
{
	return glm::vec3(light.lightPos);
}

void ForwardPass::customUI()
{
	if (ImGui::CollapsingHeader("Forward Pass", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat3("Light Position", glm::value_ptr(light.lightPos), -10.0f, 1000.0f);
		ImGui::ColorEdit4("Light Color", glm::value_ptr(light.lightColor));
	}
}