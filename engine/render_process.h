#pragma once

#include "vulkan/vulkan.hpp"

class GPUProgram;

class RenderProcess final {
public:
	vk::Pipeline pipeline;
	vk::PipelineLayout layout;
	vk::RenderPass renderPass;
	void InitPipelineLayout();
	void InitRenderPass();
	void InitPipeline(GPUProgram*);
	~RenderProcess();
	void DestroyPipeline();
};