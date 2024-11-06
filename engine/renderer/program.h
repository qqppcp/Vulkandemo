#pragma once
#include "vulkan/vulkan.hpp"
#include "Shader.h"
#include <vector>
#include <memory>

class GPUProgram final {
public:
	GPUProgram(std::string comp);
	GPUProgram(std::string vertex, std::string fragment);
	GPUProgram(std::string vertex, std::string geometry, std::string fragment);
	~GPUProgram();
	std::shared_ptr<Shader> Vertex;
	std::shared_ptr<Shader> TessellationControl;
	std::shared_ptr<Shader> TessellationEvaluation;
	std::shared_ptr<Shader> Geometry;
	std::shared_ptr<Shader> Fragment;
	std::shared_ptr<Shader> Compute;

	std::vector<vk::PipelineShaderStageCreateInfo> stages;
};