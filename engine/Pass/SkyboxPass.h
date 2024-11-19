#pragma once
#include "RenderTarget.h"
#include <vector>

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
	void init(std::vector<RenderTarget>& rts);
	void render(vk::CommandBuffer cmdbuf, uint32_t index);
	std::shared_ptr<Texture> skyboxTexture() { return hdrCubeTexture; }
	std::shared_ptr<Pipeline> pipeline() const { return m_pipeline; }
	std::shared_ptr<RenderPass> renderPass() const { return m_renderPass; }
	vk::Framebuffer framebuffer(int index) const
	{
		return framebuffers[index];
	}
private:
	std::shared_ptr<Pipeline> m_pipeline;
	std::shared_ptr<RenderPass> m_renderPass;
	std::vector<vk::Framebuffer> framebuffers;
	std::shared_ptr<GPUProgram> skyboxShader;
	std::shared_ptr<Texture> hdrCubeTexture;
	std::shared_ptr<Buffer> vertexBuffer;
	std::shared_ptr<Sampler> sampler;
	uint32_t width = 0;
	uint32_t height = 0;
};