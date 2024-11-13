#pragma once
#include <vulkan/vulkan.hpp>
class Pipeline;
class Texture;
class Sampler;
class Buffer;

class SSRIntersectPass
{
public:
	SSRIntersectPass();
	~SSRIntersectPass();
	void init(std::shared_ptr<Texture> gBufferNormal,
		std::shared_ptr<Texture> gBufferSpecular,
		std::shared_ptr<Texture> gBufferBaseColor,
		std::shared_ptr<Texture> hierarchicalDepth,
		std::shared_ptr<Texture> noiseTexture);
	void run(vk::CommandBuffer cmdbuf);
	std::shared_ptr<Texture> intersectTexture() { return outSSRIntersectTexture; }
private:
	std::shared_ptr<Pipeline> m_pipeline;
	std::shared_ptr<Texture> outSSRIntersectTexture;
	std::shared_ptr<Texture> gBufferNormal;
	std::shared_ptr<Texture> gBufferSpecular;
	std::shared_ptr<Texture> gBufferBaseColor;
	std::shared_ptr<Texture> hierarchicalDepth;
	std::shared_ptr<Texture> noiseTexture;
	std::shared_ptr<Sampler> sampler;
	std::shared_ptr<Buffer> cameraBuffer;
	std::shared_ptr<Buffer> stageBuffer;
	void* ptr{ nullptr };

	uint32_t width;
	uint32_t height;
	uint32_t index = 0;
};