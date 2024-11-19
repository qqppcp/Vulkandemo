#pragma once
#include <vector>
#include "Pipeline.h"
#include "ImGuiBase.h"

class Texture;
class RenderPass;
class GPUProgram;

class GBufferPass : public ImGuiBase
{
public:
	struct GBufferPushConstants 
	{
		uint32_t applyJitter;
	};

	GBufferPass();
	~GBufferPass();
	void init(unsigned int width, unsigned int height);

	void render(const std::vector<Pipeline::SetAndBindingIndex>& sets,
		vk::Buffer indexBuffer, vk::Buffer indirectDrawBuffer,
		vk::Buffer indirectDrawCountBuffer, uint32_t numMeshes, uint32_t bufferSize,
		bool applyJitter = false);

	std::shared_ptr<Pipeline> pipeline() const { return m_pipeline; }

	std::shared_ptr<Texture> baseColorTexture() const {
		return gBufferBaseColorTexture;
	}

	std::shared_ptr<Texture> positionTexture() const {
		return gBufferPositionTexture;
	}

	std::shared_ptr<Texture> normalTexture() const {
		return gBufferNormalTexture;
	}

	std::shared_ptr<Texture> emissiveTexture() const {
		return gBufferEmissiveTexture;
	}

	std::shared_ptr<Texture> specularTexture() const {
		return gBufferSpecularTexture;
	}

	std::shared_ptr<Texture> velocityTexture() const {
		return gBufferVelocityTexture;
	}

	std::shared_ptr<Texture> depthTexture() const { return m_depthTexture; }

	void setImageId(void* id) { this->id = id; }

	virtual void customUI() override;

private:
	void initTextures(unsigned int width, unsigned int height);
private:
	std::shared_ptr<Texture> gBufferBaseColorTexture;
	std::shared_ptr<Texture> gBufferNormalTexture;
	std::shared_ptr<Texture> gBufferEmissiveTexture;
	std::shared_ptr<Texture> gBufferSpecularTexture;
	std::shared_ptr<Texture> gBufferPositionTexture;
	std::shared_ptr<Texture> gBufferVelocityTexture;
	std::shared_ptr<Texture> m_depthTexture;

	std::shared_ptr<RenderPass> renderPass;
	vk::Framebuffer m_framebuffer;
	std::shared_ptr<Pipeline> m_pipeline;
	std::shared_ptr<GPUProgram> gBufferShader;

	void* id;
};