#include "LightBoxPass.h"
#include "render_process.h"
#include "Pipeline.h"
#include "Context.h"
#include "program.h"
#include "geometry.h"
#include "Texture.h"
#include "Buffer.h"
#include "camera.h"
#include "mesh.h"
#include "define.h"

constexpr uint32_t STORAGE_BUFFER_SET = 0;

LightBoxPass::LightBoxPass() {}

LightBoxPass::~LightBoxPass()
{
	for (auto framebuffer : framebuffers)
	{
		Context::GetInstance().device.destroyFramebuffer(framebuffer);
	}
}

void LightBoxPass::init(std::vector<RenderTarget>& rts)
{
	width = rts[0].width;
	height = rts[0].height;
	auto device = Context::GetInstance().device;
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	glm::vec3 minPos = glm::vec3(-0.5f, -0.5f, -0.5f);
	glm::vec3 maxPos = glm::vec3( 0.5f,  0.5f,  0.5f);
	std::vector<glm::vec3> positions = {
				minPos,
				glm::vec3(maxPos.x, minPos.y, minPos.z),
				glm::vec3(maxPos.x, maxPos.y, minPos.z),
				glm::vec3(minPos.x, maxPos.y, minPos.z),
				glm::vec3(minPos.x, minPos.y, maxPos.z),
				glm::vec3(maxPos.x, minPos.y, maxPos.z),
				glm::vec3(maxPos.x, maxPos.y, maxPos.z),
				glm::vec3(minPos.x, maxPos.y, maxPos.z)
	};
	for (int j = 0; j < 8; j++)
	{
		Vertex vert;
		vert.Position = positions[j];
		vertices.push_back(vert);
	}
	std::vector<unsigned int> index = {
		// Front face
		0, 2, 1, 0, 3, 2,
		// Back face
		4, 5, 6, 4, 6, 7,
		// Left face
		0, 4, 7, 0, 7, 3,
		// Right face
		1, 6, 5, 1, 2, 6,
		// Top face
		3, 6, 2, 3, 7, 6,
		// Bottom face
		0, 1, 5, 0, 5, 4
	};

	for (int j = 0; j < 36; j++)
	{
		indices.push_back(index[j]);
	}

	vertexBuffer.reset(new Buffer(vertices.size() * sizeof(Vertex), vk::BufferUsageFlagBits::eStorageBuffer |
		vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress |
		vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal));
	indexBuffer.reset(new Buffer(indices.size() * sizeof(uint32_t), vk::BufferUsageFlagBits::eStorageBuffer |
		vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress |
		vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal));
	UploadBufferData({}, vertexBuffer, vertexBuffer->size, vertices.data());
	UploadBufferData({}, indexBuffer, indexBuffer->size, indices.data());
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
	auto shader = std::make_shared<GPUProgram>(shaderPath + "aabb.vert.spv", shaderPath + "aabb.frag.spv");
	std::vector<Pipeline::SetDescriptor> setLayouts;
	{
		Pipeline::SetDescriptor set;
		set.set = STORAGE_BUFFER_SET;
		vk::DescriptorSetLayoutBinding binding;
		binding.setBinding(0)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
		set.bindings.push_back(binding);
		binding.setBinding(1)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
		set.bindings.push_back(binding);
		setLayouts.push_back(set);
	}
	std::vector<vk::PushConstantRange> ranges(1);
	ranges[0].setOffset(0)
		.setSize(sizeof(glm::mat4) * 3)
		.setStageFlags(vk::ShaderStageFlagBits::eVertex);
	const Pipeline::GraphicsPipelineDescriptor gpDesc = {
			.sets = setLayouts,
			.vertexShader = shader->Vertex,
			.fragmentShader = shader->Fragment,
			.pushConstants = {ranges},
			.dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor },
			.colorTextureFormats = {Context::GetInstance().swapchain->info.surfaceFormat.format},
			.depthTextureFormat = vk::Format::eD24UnormS8Uint,
			.cullMode = vk::CullModeFlagBits::eBack,
			.viewport = vk::Viewport {0, 0, 0, 0},
			.depthTestEnable = true,
			.depthWriteEnable = true,
			.depthCompareOperation = vk::CompareOp::eLess,
	};
	m_pipeline.reset(new Pipeline(gpDesc, m_renderPass->vkRenderPass()));
	m_pipeline->allocateDescriptors({
		{.set = STORAGE_BUFFER_SET, .count = 1},
		});
	m_pipeline->bindResource(STORAGE_BUFFER_SET, 0, 0, vertexBuffer, 0, vertexBuffer->size, vk::DescriptorType::eStorageBuffer);
	m_pipeline->bindResource(STORAGE_BUFFER_SET, 1, 0, indexBuffer, 0, indexBuffer->size, vk::DescriptorType::eStorageBuffer);
}

void LightBoxPass::render(vk::CommandBuffer cmdbuf, uint32_t index, glm::vec3 lightPos)
{
	
	std::array<glm::mat4, 3> c;
	c[0] = glm::mat4(1.0f);
	c[0] = glm::translate(c[0], lightPos);
	c[0] = glm::scale(c[0], glm::vec3(0.1f));
	c[1] = CameraManager::mainCamera->GetViewMatrix();
	c[2] = glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 1000.0f);
	vk::RenderPassBeginInfo renderPassBI;
	renderPassBI.setRenderPass(m_renderPass->vkRenderPass())
		.setFramebuffer(framebuffers[index])
		.setRenderArea(VkRect2D({ 0,0 }, { width, height }));
	cmdbuf.beginRenderPass(renderPassBI, vk::SubpassContents::eInline);
	cmdbuf.setViewport(0, { vk::Viewport{ 0, (float)height, (float)width, -(float)height, 0.0f, 1.0f} });
	cmdbuf.setScissor(0, { vk::Rect2D{vk::Offset2D{0, 0}, vk::Extent2D{ width, height } } });
	m_pipeline->bind(cmdbuf);
	m_pipeline->bindDescriptorSets(cmdbuf, {
		{.set = STORAGE_BUFFER_SET, .bindIdx = 0},
		});
	m_pipeline->updatePushConstant(cmdbuf, vk::ShaderStageFlagBits::eVertex, sizeof(glm::mat4) * 3, c.data());
	cmdbuf.bindIndexBuffer(indexBuffer->buffer, 0, vk::IndexType::eUint32);
	cmdbuf.drawIndexed(indexBuffer->size / sizeof(uint32_t), 1, 0, 0, 0);
	cmdbuf.endRenderPass();
}
