#pragma once
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

class RenderPass;
class Pipeline;
class Texture;
class Buffer;
struct Mesh;

class LightBoxPass
{
public:
	LightBoxPass();
	~LightBoxPass();
	void init(std::shared_ptr<Texture> colorTexture, std::shared_ptr<Texture> depthTexture);
	void render(vk::CommandBuffer cmdbuf, uint32_t index, glm::vec3 lightPos);
	std::shared_ptr<Pipeline> pipeline() const { return m_pipeline; }
	std::shared_ptr<Texture> colorTexture() { return colorTexture_; }
	std::shared_ptr<Texture> depthTexture() { return depthTexture_; }
private:
	std::shared_ptr<RenderPass> m_renderPass;
	std::shared_ptr<Pipeline> m_pipeline;
	vk::Framebuffer framebuffer;

	std::shared_ptr<Buffer> indexBuffer;
	std::shared_ptr<Buffer> vertexBuffer;
	std::shared_ptr<Texture> colorTexture_;
	std::shared_ptr<Texture> depthTexture_;
	uint32_t width = 0;
	uint32_t height = 0;
};