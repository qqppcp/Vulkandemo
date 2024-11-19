#pragma once
#include "RenderTarget.h"
#include "ImGuiBase.h"
#include <vector>
#include <glm/glm.hpp>

class Pipeline;
class RenderPass;
class GPUProgram;
class Texture;
class Sampler;
class Buffer;

class ForwardPass : public ImGuiBase
{
public:
	ForwardPass() = default;
	~ForwardPass();
	void init(std::vector<RenderTarget>& rts);
	void render(vk::CommandBuffer cmdbuf, uint32_t index, vk::Buffer indexBuffer, 
		vk::Buffer indirectDrawBuffer, vk::Buffer indirectDrawCountBuffer,
		uint32_t numMeshes, uint32_t bufferSize);

	glm::vec3 LightPos();
	std::shared_ptr<Pipeline> pipeline() const { return m_pipeline; }

	virtual void customUI() override;

private:
	std::shared_ptr<Pipeline> m_pipeline;
	std::shared_ptr<RenderPass> m_renderPass;
	std::vector<vk::Framebuffer> framebuffers;
	std::shared_ptr<GPUProgram> blinnShader;

	std::shared_ptr<Buffer> cameraBuffer;
	std::shared_ptr<Buffer> lightBuffer;
	std::shared_ptr<Buffer> stageBuffer1;
	std::shared_ptr<Buffer> stageBuffer2;
	void* ptr1;
	void* ptr2;

	uint32_t width = 0;
	uint32_t height = 0;
};