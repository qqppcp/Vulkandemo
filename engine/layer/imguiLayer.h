#pragma once
#include "layer.h"
#include "ImGuiState.h"
#include <memory>

class RenderPass;
class ImGuiBase;

class ImGuiLayer : public Layer
{
public:
	ImGuiLayer();
	~ImGuiLayer();
	virtual 
	virtual void OnRender() override;
	virtual void OnAttach() override;
	virtual void OnDetach() override;
	void addUI(ImGuiBase* pUI);

private:
	ImGuiState imguistate;
	std::shared_ptr<RenderPass> renderPass;
	static constexpr uint32_t COLOR_SET = 0;
	static constexpr uint32_t TEXTURES_AND_SAMPLER_SET = 1;
	static constexpr uint32_t VERTEX_INDEX_SET = 2;
	static constexpr uint32_t MATERIAL_SET = 3;
};