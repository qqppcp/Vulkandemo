#pragma once
#include "RenderTarget.h"
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
	void init(std::vector<RenderTarget>& rts);
	void render(vk::CommandBuffer cmdbuf, uint32_t index, glm::vec3 lightPos);
	std::shared_ptr<Pipeline> pipeline() const { return m_pipeline; }
private:
	std::shared_ptr<RenderPass> m_renderPass;
	std::shared_ptr<Pipeline> m_pipeline;
	std::vector<vk::Framebuffer> framebuffers;

	std::shared_ptr<Buffer> indexBuffer;
	std::shared_ptr<Buffer> vertexBuffer;

	uint32_t width = 0;
	uint32_t height = 0;
};