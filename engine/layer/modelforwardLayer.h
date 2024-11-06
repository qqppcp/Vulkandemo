#pragma once
#include "layer.h"
#include "ImGuiBase.h"
#include <glm/glm.hpp>
#include <memory>

class GPUProgram;
class Sampler;
class Buffer;
class RenderPass;
class Pipeline;
struct Mesh;

class ModelForwardLayer : public Layer, public ImGuiBase
{
public:
	ModelForwardLayer(std::string_view path);
	~ModelForwardLayer();
	virtual void OnUpdate(float _deltatime = 0) override;
	virtual void OnRender() override;
	std::shared_ptr<RenderPass> getRenderPass() { return renderPass; }
	virtual void customUI() override;

private:
	glm::vec4 color;
	bool isOpen{ true };
	std::unique_ptr<GPUProgram> blinn_phong;
	std::shared_ptr<Sampler> sampler;
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Buffer> stageBuffer;
	std::shared_ptr<Buffer> vertexBuffer;
	std::shared_ptr<Buffer> indiceBuffer;
	std::shared_ptr<Buffer> materialBuffer;
	std::shared_ptr<Buffer> uniformBuffer;
	std::shared_ptr<RenderPass> renderPass;
	std::shared_ptr<Pipeline> pipeline;
	static constexpr uint32_t COLOR_SET = 0;
	static constexpr uint32_t TEXTURES_AND_SAMPLER_SET = 1;
	static constexpr uint32_t VERTEX_INDEX_SET = 2;
	static constexpr uint32_t MATERIAL_SET = 3;
};