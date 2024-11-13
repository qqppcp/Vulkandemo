#pragma once
#include <memory>
#include <vulkan/vulkan.hpp>

class Pipeline;
class Sampler;
class Texture;

class HierarchicalDepthBufferPass
{
public:
	HierarchicalDepthBufferPass();
	~HierarchicalDepthBufferPass();
	void init(std::shared_ptr<Texture> depthTexture);
	void generateHierarchicalDepthBuffer(vk::CommandBuffer cmdbuf);
	std::shared_ptr<Texture> hierarchicalDepthTexture() { return outHierarchicalDepthTexture; }
private:
	std::shared_ptr<Pipeline> m_pipeline;
	std::shared_ptr<Texture> outHierarchicalDepthTexture;
	std::vector<vk::ImageView> hierarchicalDepthTexturePerMipImageViews;
	std::shared_ptr<Texture> depthTexture;
	std::shared_ptr<Sampler> sampler;
};