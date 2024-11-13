#pragma once
#include <vulkan/vulkan.hpp>


class Pipeline;
class Sampler;
class Texture;

class SSAOPass
{
public:
	SSAOPass();
	~SSAOPass();
	void init(std::shared_ptr<Texture> gBufferDepth);
	void run(vk::CommandBuffer cmdbuf);
	std::shared_ptr<Texture> ssaoTexture() { return outSSAOTexture; }
private:
	std::shared_ptr<Pipeline> m_pipeline;
	std::shared_ptr<Texture> outSSAOTexture;
	std::shared_ptr<Texture> gBufferDepth;
	std::shared_ptr<Sampler> sampler;
};