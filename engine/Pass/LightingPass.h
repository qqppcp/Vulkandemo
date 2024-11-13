#pragma once
#include <vulkan/vulkan.hpp>
#include "LightData.h"

class RenderPass;
class Pipeline;
class Texture;
class Sampler;
class Buffer;

class LightingPass
{
public:
	LightingPass();
	~LightingPass();
	void init(std::shared_ptr<Texture> gBufferNormal,
		std::shared_ptr<Texture> gBufferSpecular,
		std::shared_ptr<Texture> gBufferBaseColor,
		std::shared_ptr<Texture> gBufferPosition,
		std::shared_ptr<Texture> gBufferDepth,
		std::shared_ptr<Texture> ambientOcclusion,
		std::shared_ptr<Texture> shadowDepth);
	void render(vk::CommandBuffer cmdbuf, uint32_t index, const LightData& data,
		const glm::mat4& viewMat, const glm::mat4& projMat);
	
	std::shared_ptr<Pipeline> pipeline() const { return m_pipeline; }
	std::shared_ptr<RenderPass> renderPass() const { return m_renderPass; }
	std::shared_ptr<Texture> lightTexture() const { return outLightingTexture; }
private:
	std::shared_ptr<RenderPass> m_renderPass;
	std::shared_ptr<Pipeline> m_pipeline;
	vk::Framebuffer frameBuffer;

	std::shared_ptr<Texture> outLightingTexture;
	std::shared_ptr<Texture> gBufferNormal;
	std::shared_ptr<Texture> gBufferSpecular;
	std::shared_ptr<Texture> gBufferBaseColor;
	std::shared_ptr<Texture> gBufferDepth;
	std::shared_ptr<Texture> gBufferPosition;
	std::shared_ptr<Texture> ambientOcclusion;
	std::shared_ptr<Texture> shadowDepth;
	std::shared_ptr<Sampler> sampler;
	std::shared_ptr<Sampler> samplerShadowMap;
	std::shared_ptr<Buffer> cameraBuffer;
	std::shared_ptr<Buffer> lightBuffer;
	std::shared_ptr<Buffer> stageBuffer1;
	std::shared_ptr<Buffer> stageBuffer2;
	void* ptr1;
	void* ptr2;

	uint32_t width = 0;
	uint32_t height = 0;
};