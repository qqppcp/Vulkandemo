#pragma once
#include <vulkan/vulkan.hpp>

class Buffer;
class Pipeline;
class Texture;

class NoisePass
{
public:
	NoisePass();
	void init();
	void generateNoise(vk::CommandBuffer cmdbuf);
	std::shared_ptr<Texture> noiseTexture() { return outNoiseTexture; }
private:
	std::shared_ptr<Pipeline> m_pipeline;
	std::shared_ptr<Texture> outNoiseTexture;
	std::shared_ptr<Buffer> sobolBuffer;
	std::shared_ptr<Buffer> rankingTileBuffer;
	std::shared_ptr<Buffer> scramblingTileBuffer;

	uint32_t index = 0;
};