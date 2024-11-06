#pragma once
#include <vulkan/vulkan.hpp>

class Pipeline;
class RenderPass;
class GPUProgram;

class FullScreenPass
{
public:
	FullScreenPass(bool useDynamicRendering = false);
	~FullScreenPass();
	void init(std::vector<vk::Format> colorTextureFormats);
	void render(uint32_t index, bool showAsDepth = false);
	std::shared_ptr<Pipeline> pipeline() const { return m_pipeline; }
	std::shared_ptr<RenderPass> renderPass() const { return m_renderPass; }
	vk::Framebuffer framebuffer(int index) const
	{
		return m_framebuffers[index];
	}
private:
	std::shared_ptr<Pipeline> m_pipeline;
	std::shared_ptr<RenderPass> m_renderPass;
	std::vector<vk::Framebuffer> m_framebuffers;
	std::shared_ptr<GPUProgram> fullScreenShader;
	uint32_t width;
	uint32_t height;
	bool bUseDynamicRendering = false;
};