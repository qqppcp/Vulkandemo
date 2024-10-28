#pragma once
#include "layer.h"
#include <glm/glm.hpp>
#include <memory>

class GPUProgram;
class Sampler;
class Buffer;
class RenderPass;
class Pipeline;
class Texture;
struct Mesh;

class SkyboxLayer : public Layer
{
public:
	SkyboxLayer(std::string_view path);
	~SkyboxLayer();
	virtual void OnRender() override;
private:
	std::unique_ptr<GPUProgram> skybox;
	std::shared_ptr<Sampler> sampler;
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Buffer> vertexBuffer;
	std::shared_ptr<RenderPass> renderPass;
	std::shared_ptr<Pipeline> pipeline;
	std::shared_ptr<Texture> hdrT;
	static constexpr uint32_t TEXTURES_AND_SAMPLER_SET = 0;
	static constexpr uint32_t VERTEX_INDEX_SET = 1;
};