#pragma once
#include "ImGuiBase.h"
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

class Pipeline;
class RenderPass;
class GPUProgram;
class Texture;
class Sampler;
class Buffer;

class ForwardPass : public ImGuiBase
{
public:
	ForwardPass() = default;
	~ForwardPass();
	void init(std::shared_ptr<Texture> color, std::shared_ptr<Texture> depth);
	void render(vk::CommandBuffer cmdbuf, uint32_t index, vk::Buffer indexBuffer, 
		vk::Buffer indirectDrawBuffer, vk::Buffer indirectDrawCountBuffer,
		uint32_t numMeshes, uint32_t bufferSize);

	glm::vec3 LightPos();
	std::shared_ptr<Pipeline> pipeline() const { return m_pipeline; }
	virtual void customUI() override;
	std::shared_ptr<Texture> colorTexture() { return colorTexture_; }
	std::shared_ptr<Texture> depthTexture() { return depthTexture_; }

private:
	std::shared_ptr<Pipeline> m_pipeline;
	std::shared_ptr<RenderPass> m_renderPass;
	vk::Framebuffer framebuffer;
	std::shared_ptr<GPUProgram> blinnShader;

	std::shared_ptr<Texture> colorTexture_;
	std::shared_ptr<Texture> depthTexture_;

	std::shared_ptr<Buffer> cameraBuffer;
	std::shared_ptr<Buffer> lightBuffer;
	std::shared_ptr<Buffer> stageBuffer1;
	std::shared_ptr<Buffer> stageBuffer2;
	void* ptr1;
	void* ptr2;

	uint32_t width = 0;
	uint32_t height = 0;
};