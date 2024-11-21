#pragma once
#include <vulkan/vulkan.hpp>

class Texture;
class Pipeline;
class RenderPass;

class ClearPass
{
public:
	~ClearPass();
	void init(std::shared_ptr<Texture> color, std::shared_ptr<Texture> depth);
	void render(vk::CommandBuffer cmdbuf, uint32_t index);
	std::shared_ptr<Texture> colorTexture() { return colorTexture_; }
	std::shared_ptr<Texture> depthTexture() { return depthTexture_; }
private:
	std::shared_ptr<Texture> colorTexture_;
	std::shared_ptr<Texture> depthTexture_;
	vk::Framebuffer framebuffer;
	std::shared_ptr<RenderPass> renderPass;
	uint32_t width = 0;
	uint32_t height = 0;
};