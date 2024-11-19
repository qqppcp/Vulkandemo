#include "LineBoxPass.h"
#include "render_process.h"
#include "Pipeline.h"
#include "Context.h"
#include "program.h"
#include "Texture.h"
#include "Buffer.h"
#include "mesh.h"
#include "define.h"

constexpr uint32_t STORAGE_BUFFER_SET = 0;

LineBoxPass::LineBoxPass() {}

LineBoxPass::~LineBoxPass()
{
	auto device = Context::GetInstance().device;
	device.destroyFramebuffer(frameBuffer);
}

void LineBoxPass::init(std::shared_ptr<Texture> inColorTexture, std::shared_ptr<Texture> inDepthTexture, std::shared_ptr<Mesh> mesh)
{
	width = inColorTexture->width;
	height = inDepthTexture->height;
	auto device = Context::GetInstance().device;
	{
		auto& aabbs = mesh->aabbs;
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		for (int i = 0; i < aabbs.size(); i++)
		{
			int vertexStart = vertices.size();
			std::vector<glm::vec3> positions = {
				aabbs[i].minPos,
				glm::vec3(aabbs[i].maxPos.x, aabbs[i].minPos.y, aabbs[i].minPos.z),
				glm::vec3(aabbs[i].maxPos.x, aabbs[i].maxPos.y, aabbs[i].minPos.z),
				glm::vec3(aabbs[i].minPos.x, aabbs[i].maxPos.y, aabbs[i].minPos.z),
				glm::vec3(aabbs[i].minPos.x, aabbs[i].minPos.y, aabbs[i].maxPos.z),
				glm::vec3(aabbs[i].maxPos.x, aabbs[i].minPos.y, aabbs[i].maxPos.z),
				glm::vec3(aabbs[i].maxPos.x, aabbs[i].maxPos.y, aabbs[i].maxPos.z),
				glm::vec3(aabbs[i].minPos.x, aabbs[i].maxPos.y, aabbs[i].maxPos.z)
			};
			for (int j = 0; j < 8; j++)
			{
				Vertex vert;
				vert.Position = positions[j];
				vertices.push_back(vert);
			}
			std::vector<unsigned int> index = {
				// Front face
				0, 1, 2, 0, 2, 3,
				// Back face
				4, 5, 6, 4, 6, 7,
				// Left face
				0, 4, 7, 0, 7, 3,
				// Right face
				1, 5, 6, 1, 6, 2,
				// Top face
				3, 2, 6, 3, 6, 7,
				// Bottom face
				0, 1, 5, 0, 5, 4
			};
			
			for (int j = 0; j < 36; j++)
			{
				indices.push_back(vertexStart + index[j]);
			}
		}
		vertexBuffer.reset(new Buffer(vertices.size() * sizeof(Vertex), vk::BufferUsageFlagBits::eStorageBuffer |
			vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress |
			vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal));
		indexBuffer.reset(new Buffer(indices.size() * sizeof(uint32_t), vk::BufferUsageFlagBits::eStorageBuffer |
			vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress |
			vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal));
		UploadBufferData({}, vertexBuffer, vertexBuffer->size, vertices.data());
		UploadBufferData({}, indexBuffer, indexBuffer->size, indices.data());
	}
	m_renderPass.reset(new RenderPass(std::vector<vk::Format>{inColorTexture->format,  vk::Format::eD24UnormS8Uint},
		std::vector<vk::ImageLayout>{vk::ImageLayout::eUndefined,  vk::ImageLayout::eUndefined},
		std::vector<vk::ImageLayout>{vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, },
		std::vector<vk::AttachmentLoadOp>{vk::AttachmentLoadOp::eDontCare,  vk::AttachmentLoadOp::eDontCare},
		std::vector<vk::AttachmentStoreOp>{vk::AttachmentStoreOp::eStore, vk::AttachmentStoreOp::eStore},
		vk::PipelineBindPoint::eGraphics, {}, 1));
	{
		std::vector<vk::ImageView> attachments{ inColorTexture->view, inDepthTexture->view };
		vk::FramebufferCreateInfo createInfo;
		createInfo.setAttachments(attachments)
			.setHeight(height)
			.setWidth(width)
			.setRenderPass(m_renderPass->vkRenderPass())
			.setLayers(1);
		frameBuffer = Context::GetInstance().device.createFramebuffer(createInfo);
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
			.polygonMode = vk::PolygonMode::eLine,
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

void LineBoxPass::render(vk::CommandBuffer cmdbuf, const glm::mat4& viewMat, const glm::mat4& projMat)
{
	std::array<glm::mat4, 3> c;
	c[0] = glm::mat4(1.0f);
	c[1] = viewMat;
	c[2] = projMat;
	vk::RenderPassBeginInfo renderPassBI;
	renderPassBI.setRenderPass(m_renderPass->vkRenderPass())
		.setFramebuffer(frameBuffer)
		.setRenderArea(VkRect2D({ 0,0 }, { width, height }));
	cmdbuf.beginRenderPass(renderPassBI, vk::SubpassContents::eInline);
	cmdbuf.setViewport(0, { vk::Viewport{ 0, (float)height, (float)width, -(float)height, 0.0f, 1.0f}});
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
