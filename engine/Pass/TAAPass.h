#pragma once
#include <vulkan/vulkan.hpp>

class Sampler;
class Texture;
class Pipeline;
class GPUProgram;

class TAAPass
{
public:
	struct TAAPushConstants
	{
		uint32_t isFirstFrame;
		uint32_t isCameraMoving;
	};
	TAAPass() = default;
	void init(std::shared_ptr<Texture> depthTexture,
		std::shared_ptr<Texture> velocityTexture,
		std::shared_ptr<Texture> colorTexture);
	void doAA(vk::CommandBuffer cmdbuf, uint32_t index, int isCamMoving);
	std::shared_ptr<Texture> ColorTexture() const { return outColorTexture; }
private:
	void initSharpenPipeline();
	std::shared_ptr<Sampler> sampler;
	std::shared_ptr<Sampler> pointSampler;
	
	std::shared_ptr<Texture> depthTexture;
	std::shared_ptr<Texture> historyTexture;
	std::shared_ptr<Texture> velocityTexture;
	std::shared_ptr<Texture> colorTexture;

	std::shared_ptr<Texture> outColorTexture;
	std::shared_ptr<Pipeline> pipeline;
	std::shared_ptr<Pipeline> sharpenPipeline;

	uint32_t width;
	uint32_t height;
};