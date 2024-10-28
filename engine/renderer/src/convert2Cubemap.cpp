#include "convert2Cubemap.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Texture.h"
#include "Pipeline.h"
#include "render_process.h"
#include "CommandBuffer.h"
#include "program.h"
#include "Context.h"
#include "Sampler.h"
#include "define.h"
#include "mesh.h"

std::shared_ptr<Texture> convert2Cubemap(std::shared_ptr<Texture> flatTex)
{
	constexpr uint32_t TEXTURES_AND_SAMPLER_SET = 0;
	constexpr uint32_t VERTEX_INDEX_SET = 1;
	Mesh mesh;
	mesh.vertices.resize(36);
	{
		mesh.vertices[0].Position = { -1.0f, -1.0f, -1.0f }; mesh.vertices[0].Normal = { 0.0f,  0.0f, -1.0f }; mesh.vertices[0].TexCoords = { 0.0f, 0.0f };
		mesh.vertices[1].Position = { 1.0f,  1.0f, -1.0f }; mesh.vertices[0].Normal = { 0.0f,  0.0f, -1.0f }; mesh.vertices[0].TexCoords = { 1.0f, 1.0f };
		mesh.vertices[2].Position = { 1.0f, -1.0f, -1.0f }; mesh.vertices[0].Normal = { 0.0f,  0.0f, -1.0f }; mesh.vertices[0].TexCoords = { 1.0f, 0.0f };
		mesh.vertices[3].Position = { 1.0f,  1.0f, -1.0f }; mesh.vertices[0].Normal = { 0.0f,  0.0f, -1.0f }; mesh.vertices[0].TexCoords = { 1.0f, 1.0f };
		mesh.vertices[4].Position = { -1.0f, -1.0f, -1.0f }; mesh.vertices[0].Normal = { 0.0f,  0.0f, -1.0f }; mesh.vertices[0].TexCoords = { 0.0f, 0.0f };
		mesh.vertices[5].Position = { -1.0f,  1.0f, -1.0f }; mesh.vertices[0].Normal = { 0.0f,  0.0f, -1.0f }; mesh.vertices[0].TexCoords = { 0.0f, 1.0f };

		mesh.vertices[6].Position = { -1.0f, -1.0f,  1.0f }; mesh.vertices[0].Normal = { 0.0f,  0.0f, 1.0f }; mesh.vertices[0].TexCoords = { 0.0f, 0.0f };
		mesh.vertices[7].Position = { 1.0f, -1.0f,  1.0f }; mesh.vertices[0].Normal = { 0.0f,  0.0f, 1.0f }; mesh.vertices[0].TexCoords = { 1.0f, 0.0f };
		mesh.vertices[8].Position = { 1.0f,  1.0f,  1.0f }; mesh.vertices[0].Normal = { 0.0f,  0.0f, 1.0f }; mesh.vertices[0].TexCoords = { 1.0f, 1.0f };
		mesh.vertices[9].Position = { 1.0f,  1.0f,  1.0f }; mesh.vertices[0].Normal = { 0.0f,  0.0f, 1.0f }; mesh.vertices[0].TexCoords = { 1.0f, 1.0f };
		mesh.vertices[10].Position = { -1.0f,  1.0f,  1.0f }; mesh.vertices[0].Normal = { 0.0f,  0.0f, 1.0f }; mesh.vertices[0].TexCoords = { 0.0f, 1.0f };
		mesh.vertices[11].Position = { -1.0f, -1.0f,  1.0f }; mesh.vertices[0].Normal = { 0.0f,  0.0f, 1.0f }; mesh.vertices[0].TexCoords = { 0.0f, 0.0f };

		mesh.vertices[12].Position = { -1.0f,  1.0f,  1.0f }; mesh.vertices[0].Normal = { -1.0f,  0.0f,  0.0f }; mesh.vertices[0].TexCoords = { 1.0f, 0.0f };
		mesh.vertices[13].Position = { -1.0f,  1.0f, -1.0f }; mesh.vertices[0].Normal = { -1.0f,  0.0f,  0.0f }; mesh.vertices[0].TexCoords = { 1.0f, 1.0f };
		mesh.vertices[14].Position = { -1.0f, -1.0f, -1.0f }; mesh.vertices[0].Normal = { -1.0f,  0.0f,  0.0f }; mesh.vertices[0].TexCoords = { 0.0f, 1.0f };
		mesh.vertices[15].Position = { -1.0f, -1.0f, -1.0f }; mesh.vertices[0].Normal = { -1.0f,  0.0f,  0.0f }; mesh.vertices[0].TexCoords = { 0.0f, 1.0f };
		mesh.vertices[16].Position = { -1.0f, -1.0f,  1.0f }; mesh.vertices[0].Normal = { -1.0f,  0.0f,  0.0f }; mesh.vertices[0].TexCoords = { 0.0f, 0.0f };
		mesh.vertices[17].Position = { -1.0f,  1.0f,  1.0f }; mesh.vertices[0].Normal = { -1.0f,  0.0f,  0.0f }; mesh.vertices[0].TexCoords = { 1.0f, 0.0f };

		mesh.vertices[18].Position = { 1.0f,  1.0f,  1.0f }; mesh.vertices[0].Normal = { 1.0f,  0.0f,  0.0f }; mesh.vertices[0].TexCoords = { 1.0f, 0.0f };
		mesh.vertices[19].Position = { 1.0f, -1.0f, -1.0f }; mesh.vertices[0].Normal = { 1.0f,  0.0f,  0.0f }; mesh.vertices[0].TexCoords = { 0.0f, 1.0f };
		mesh.vertices[20].Position = { 1.0f,  1.0f, -1.0f }; mesh.vertices[0].Normal = { 1.0f,  0.0f,  0.0f }; mesh.vertices[0].TexCoords = { 1.0f, 1.0f };
		mesh.vertices[21].Position = { 1.0f, -1.0f, -1.0f }; mesh.vertices[0].Normal = { 1.0f,  0.0f,  0.0f }; mesh.vertices[0].TexCoords = { 0.0f, 1.0f };
		mesh.vertices[22].Position = { 1.0f,  1.0f,  1.0f }; mesh.vertices[0].Normal = { 1.0f,  0.0f,  0.0f }; mesh.vertices[0].TexCoords = { 1.0f, 0.0f };
		mesh.vertices[23].Position = { 1.0f, -1.0f,  1.0f }; mesh.vertices[0].Normal = { 1.0f,  0.0f,  0.0f }; mesh.vertices[0].TexCoords = { 0.0f, 0.0f };

		mesh.vertices[24].Position = { -1.0f, -1.0f, -1.0f }; mesh.vertices[0].Normal = { 0.0f, -1.0f,  0.0f }; mesh.vertices[0].TexCoords = { 0.0f, 1.0f };
		mesh.vertices[25].Position = { 1.0f, -1.0f, -1.0f }; mesh.vertices[0].Normal = { 0.0f, -1.0f,  0.0f }; mesh.vertices[0].TexCoords = { 1.0f, 1.0f };
		mesh.vertices[26].Position = { 1.0f, -1.0f,  1.0f }; mesh.vertices[0].Normal = { 0.0f, -1.0f,  0.0f }; mesh.vertices[0].TexCoords = { 1.0f, 0.0f };
		mesh.vertices[27].Position = { 1.0f, -1.0f,  1.0f }; mesh.vertices[0].Normal = { 0.0f, -1.0f,  0.0f }; mesh.vertices[0].TexCoords = { 1.0f, 0.0f };
		mesh.vertices[28].Position = { -1.0f, -1.0f,  1.0f }; mesh.vertices[0].Normal = { 0.0f, -1.0f,  0.0f }; mesh.vertices[0].TexCoords = { 0.0f, 0.0f };
		mesh.vertices[29].Position = { -1.0f, -1.0f, -1.0f }; mesh.vertices[0].Normal = { 0.0f, -1.0f,  0.0f }; mesh.vertices[0].TexCoords = { 0.0f, 1.0f };

		mesh.vertices[30].Position = { -1.0f,  1.0f, -1.0f }; mesh.vertices[0].Normal = { 0.0f,  1.0f,  0.0f }; mesh.vertices[0].TexCoords = { 0.0f, 1.0f };
		mesh.vertices[31].Position = { 1.0f,  1.0f , 1.0f }; mesh.vertices[0].Normal = { 0.0f,  1.0f,  0.0f }; mesh.vertices[0].TexCoords = { 1.0f, 0.0f };
		mesh.vertices[32].Position = { 1.0f,  1.0f, -1.0f }; mesh.vertices[0].Normal = { 0.0f,  1.0f,  0.0f }; mesh.vertices[0].TexCoords = { 1.0f, 1.0f };
		mesh.vertices[33].Position = { 1.0f,  1.0f,  1.0f }; mesh.vertices[0].Normal = { 0.0f,  1.0f,  0.0f }; mesh.vertices[0].TexCoords = { 1.0f, 0.0f };
		mesh.vertices[34].Position = { -1.0f,  1.0f, -1.0f }; mesh.vertices[0].Normal = { 0.0f,  1.0f,  0.0f }; mesh.vertices[0].TexCoords = { 0.0f, 1.0f };
		mesh.vertices[35].Position = { -1.0f,  1.0f,  1.0f }; mesh.vertices[0].Normal = { 0.0f,  1.0f,  0.0f }; mesh.vertices[0].TexCoords = { 0.0f, 0.0f };
	}
	for (int i = 0; i < 36; i++)
	{
		mesh.indices.push_back(i);
	}
	std::shared_ptr<Buffer> vertexBuffer;
	std::shared_ptr<Buffer> indiceBuffer;
	std::shared_ptr<Sampler> sampler;
	std::shared_ptr<GPUProgram> convert;
	std::shared_ptr<RenderPass> renderPass;
	std::vector<vk::Framebuffer> framebuffers;
	std::vector<vk::ImageView> imageViews;
	std::shared_ptr<Texture> retTex;
	std::shared_ptr<Pipeline> pipeline;
	auto device = Context::GetInstance().device;
	{
		convert.reset(new GPUProgram(shaderPath + "equirectangular_to_cubemap.vert.spv", shaderPath + "equirectangular_to_cubemap.frag.spv"));
	}
	{
		renderPass.reset(new RenderPass(std::vector<vk::Format>{vk::Format::eB10G11R11UfloatPack32 },
			std::vector<vk::ImageLayout>{vk::ImageLayout::eUndefined},
			std::vector<vk::ImageLayout>{vk::ImageLayout::eColorAttachmentOptimal },
			std::vector<vk::AttachmentLoadOp>{vk::AttachmentLoadOp::eDontCare},
			std::vector<vk::AttachmentStoreOp>{vk::AttachmentStoreOp::eStore},
			vk::PipelineBindPoint::eGraphics, {}, UINT32_MAX));
	}
	{
		retTex.reset(new Texture());
		{
			retTex->flags = vk::SampleCountFlagBits::e1;
			retTex->format = vk::Format::eB10G11R11UfloatPack32;
			retTex->is_depth = false;
			retTex->is_stencil = false;
			retTex->layout = vk::ImageLayout::eColorAttachmentOptimal;
			vk::ImageCreateInfo imageCI;
			imageCI.setFormat(vk::Format::eB10G11R11UfloatPack32)
				.setArrayLayers(6)
				.setExtent(vk::Extent3D{ 2048, 2048, 1 })
				.setImageType(vk::ImageType::e2D)
				.setInitialLayout(vk::ImageLayout::eUndefined)
				.setMipLevels(1)
				.setTiling(vk::ImageTiling::eOptimal)
				.setUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled)
				.setSamples(vk::SampleCountFlagBits::e1)
				.setSharingMode(vk::SharingMode::eExclusive)
				.setFlags(vk::ImageCreateFlagBits::eCubeCompatible);
			retTex->image = device.createImage(imageCI);

			vk::MemoryAllocateInfo allocInfo;
			auto requirements = device.getImageMemoryRequirements(retTex->image);
			allocInfo.setAllocationSize(requirements.size);

			uint32_t index = 0;
			auto properties = Context::GetInstance().physicaldevice.getMemoryProperties();
			for (size_t i = 0; i < properties.memoryTypeCount; i++)
			{
				if ((1 << i) & requirements.memoryTypeBits && properties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)
				{
					index = i;
					break;
				}
			}
			allocInfo.setMemoryTypeIndex(index);
			retTex->memory = device.allocateMemory(allocInfo);
			device.bindImageMemory(retTex->image, retTex->memory, 0);
			vk::ImageSubresourceRange range;
			range.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setBaseArrayLayer(0)
				.setBaseMipLevel(0)
				.setLevelCount(1)
				.setLayerCount(6);
			vk::ImageViewCreateInfo viewCI;
			viewCI.setViewType(vk::ImageViewType::eCube)
				.setFormat(vk::Format::eB10G11R11UfloatPack32)
				.setSubresourceRange(range)
				.setImage(retTex->image);
			retTex->view = device.createImageView(viewCI);
			{
				for (int face = 0; face < 6; face++)
				{
					vk::ImageSubresourceRange range;
					range.setAspectMask(vk::ImageAspectFlagBits::eColor)
						.setBaseArrayLayer(face)
						.setBaseMipLevel(0)
						.setLevelCount(1)
						.setLayerCount(1);
					vk::ImageViewCreateInfo viewCI;
					viewCI.setViewType(vk::ImageViewType::e2D)
						.setFormat(vk::Format::eB10G11R11UfloatPack32)
						.setSubresourceRange(range)
						.setImage(retTex->image);

					imageViews.push_back(device.createImageView(viewCI));
					vk::FramebufferCreateInfo framebufferCI;
					framebufferCI.setAttachments(imageViews.back())
						.setLayers(1)
						.setRenderPass(renderPass->vkRenderPass())
						.setWidth(2048)
						.setHeight(2048);
					framebuffers.emplace_back(device.createFramebuffer(framebufferCI));
				}
			}
		}
	}
	{
		vertexBuffer.reset(new Buffer(mesh.vertices.size() * sizeof(Vertex), vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer,
			vk::MemoryPropertyFlagBits::eDeviceLocal));
		indiceBuffer.reset(new Buffer(mesh.indices.size() * sizeof(std::uint32_t), vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndexBuffer,
			vk::MemoryPropertyFlagBits::eDeviceLocal));
		UploadBufferData({}, vertexBuffer, mesh.vertices.size() * sizeof(Vertex), mesh.vertices.data());
		UploadBufferData({}, indiceBuffer, mesh.indices.size() * sizeof(std::uint32_t), mesh.indices.data());
	}
	{
		sampler.reset(new Sampler(vk::Filter::eLinear, vk::Filter::eLinear,
			vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
			vk::SamplerAddressMode::eRepeat, 10.0f));
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
			.vertexShader = convert->Vertex,
			.fragmentShader = convert->Fragment,
			.pushConstants = { ranges },
			.dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor,
				vk::DynamicState::eDepthTestEnable},
			.colorTextureFormats = { vk::Format::eB10G11R11UfloatPack32 },
			.cullMode = vk::CullModeFlagBits::eNone,
			.frontFace = vk::FrontFace::eClockwise,
			.viewport = vk::Viewport {0, 0, 0, 0},
			.blendEnable = true,
			.depthTestEnable = true,
			.depthWriteEnable = true,
			.depthCompareOperation = vk::CompareOp::eLess,
		};
		pipeline.reset(new Pipeline(gpDesc, renderPass->vkRenderPass()));
		pipeline->allocateDescriptors({
				{.set = TEXTURES_AND_SAMPLER_SET, .count = 1},
				{.set = VERTEX_INDEX_SET, .count = 1},
			});
	}
	glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.f);
	glm::mat4 captureViews[] =
	{
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
	};
	for (int i = 0; i < 6; i++)
	{
		pipeline->bindResource(TEXTURES_AND_SAMPLER_SET, 0, 0, flatTex, sampler);
		pipeline->bindResource(VERTEX_INDEX_SET, 0, 0, vertexBuffer, 0, vertexBuffer->size, vk::DescriptorType::eStorageBuffer);

		std::array<glm::mat4, 2> c;
		c[0] = captureViews[i];
		c[1] = proj;

		vk::RenderPassBeginInfo renderPassBI;
		std::array<vk::ClearValue, 1> clear;
		clear[0].setColor({ 0.f, 0.f, 0.f, 1.0f });
		renderPassBI.setRenderPass(renderPass->vkRenderPass())
			.setFramebuffer(framebuffers[i])
			.setRenderArea(VkRect2D({ 0,0 }, { 2048, 2048 }))
			.setClearValues(clear);
		auto cmdbuf = CommandManager::BeginSingle(Context::GetInstance().graphicsCmdPool);
		cmdbuf.beginRenderPass(renderPassBI, vk::SubpassContents::eInline);
		cmdbuf.setViewport(0, { vk::Viewport{ 0, 2048, (float)2048, (float)-2048, 0.0f, 1.0f } });
		cmdbuf.setScissor(0, { vk::Rect2D{vk::Offset2D{0, 0}, vk::Extent2D{ 2048, 2048 }} });
		cmdbuf.setDepthTestEnable(VK_TRUE);
		pipeline->bind(cmdbuf);
		pipeline->bindDescriptorSets(cmdbuf,
			{ {.set = TEXTURES_AND_SAMPLER_SET, .bindIdx = 0},
			{.set = VERTEX_INDEX_SET, .bindIdx = 0} });
		cmdbuf.pushConstants(pipeline->vkPipelineLayout(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4) * 2, c.data());
		cmdbuf.draw(36, 1, 0, 0);
		cmdbuf.endRenderPass();
		{
			vk::ImageSubresourceRange range;
			range.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setBaseArrayLayer(i)
				.setBaseMipLevel(0)
				.setLayerCount(1)
				.setLevelCount(1);
			vk::ImageMemoryBarrier barrier;
			barrier.setImage(retTex->image)
				.setOldLayout(vk::ImageLayout::eColorAttachmentOptimal)
				.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
				.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
				.setSubresourceRange(range)
				.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
				.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
			cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);
		}

		CommandManager::EndSingle(Context::GetInstance().graphicsCmdPool, cmdbuf, Context::GetInstance().graphicsQueue);
	}
	vertexBuffer.reset();
	indiceBuffer.reset();
	sampler.reset();
	convert.reset();
	renderPass.reset();
	pipeline.reset();
	for (int i = 0; i < 6; i++)
	{
		device.destroyFramebuffer(framebuffers[i]);
		device.destroyImageView(imageViews[i]);
	}
	return retTex;
}
