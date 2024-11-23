#pragma once
#include <vulkan/vulkan.hpp>

class Texture;
class Pipeline;
class RenderPass;

class VelocityPass
{
public:
	~VelocityPass();
	void init(uint32_t width, uint32_t height);
	void render(vk::CommandBuffer cmdbuf, uint32_t index, vk::Buffer indexBuffer,
		vk::Buffer indirectDrawBuffer, vk::Buffer indirectDrawCountBuffer,
		uint32_t numMeshes, uint32_t bufferSize);
	std::shared_ptr<Pipeline> pipeline() const { return pipeline_; }
	std::shared_ptr<Texture> velocityTexture() { return outVelocityTexture; }
private:
	std::shared_ptr<Texture> outVelocityTexture;
	std::shared_ptr<Pipeline> pipeline_;
	std::shared_ptr<RenderPass> renderPass;
	vk::Framebuffer framebuffer;
	uint32_t width;
	uint32_t height;
};