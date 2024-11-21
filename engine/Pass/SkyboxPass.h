#pragma once
#include <vulkan/vulkan.hpp>

class Pipeline;
class RenderPass;
class GPUProgram;
class Texture;
class Sampler;
class Buffer;

class SkyboxPass
{
public:
	SkyboxPass() = default;
	~SkyboxPass();
	void init(std::shared_ptr<Texture> colorTexture, std::shared_ptr<Texture> depthTexture);
	void render(vk::CommandBuffer cmdbuf, uint32_t index);
	std::shared_ptr<Texture> skyboxTexture() { return hdrCubeTexture; }
	std::shared_ptr<Pipeline> pipeline() const { return m_pipeline; }
	std::shared_ptr<RenderPass> renderPass() const { return m_renderPass; }
	std::shared_ptr<Texture> colorTexture() { return colorTexture_; }
	std::shared_ptr<Texture> depthTexture() { return depthTexture_; }

private:
	std::shared_ptr<Pipeline> m_pipeline;
	std::shared_ptr<RenderPass> m_renderPass;
	vk::Framebuffer framebuffer;
	std::shared_ptr<GPUProgram> skyboxShader;
	std::shared_ptr<Texture> hdrCubeTexture;
	std::shared_ptr<Buffer> vertexBuffer;
	std::shared_ptr<Sampler> sampler;
	std::shared_ptr<Texture> colorTexture_;
	std::shared_ptr<Texture> depthTexture_;
	uint32_t width = 0;
	uint32_t height = 0;
};