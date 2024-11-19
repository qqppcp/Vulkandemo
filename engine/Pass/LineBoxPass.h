#pragma once
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

class RenderPass;
class Pipeline;
class Texture;
class Buffer;
struct Mesh;

class LineBoxPass
{
public:
	LineBoxPass();
	~LineBoxPass();
	void init(std::shared_ptr<Texture> inColorTexture,
		std::shared_ptr<Texture> inDepthTexture, std::shared_ptr<Mesh> mesh);
	void render(vk::CommandBuffer cmdbuf, const glm::mat4& viewMat, const glm::mat4& projMat);
	std::shared_ptr<Pipeline> pipeline() const { return m_pipeline; }
private:
	std::shared_ptr<RenderPass> m_renderPass;
	std::shared_ptr<Pipeline> m_pipeline;
	vk::Framebuffer frameBuffer;

	std::shared_ptr<Buffer> indexBuffer;
	std::shared_ptr<Buffer> vertexBuffer;

	uint32_t width = 0;
	uint32_t height = 0;
};