#pragma once
#include <vulkan/vulkan.hpp>
#include "Pipeline.h"

class Texture;
class RenderPass;
class GPUProgram;

class ShadowMapPass
{
public:
	ShadowMapPass();
	~ShadowMapPass();
	void init();
	void render(const std::vector<Pipeline::SetAndBindingIndex>& sets,
		vk::Buffer indexBuffer, vk::Buffer indirectDrawBuffer,
		uint32_t numMeshes, uint32_t bufferSize);

	std::shared_ptr<Pipeline> pipeline() { return m_pipeline; }
	std::shared_ptr<Texture> shadowmap() { return m_shadowmap; }
private:
	std::vector<vk::Framebuffer> m_framebuffers;
	std::shared_ptr<Texture> m_shadowmap;
	std::shared_ptr<RenderPass> m_renderPass;
	std::shared_ptr<Pipeline> m_pipeline;
	std::shared_ptr<GPUProgram> m_shadowShader;
};