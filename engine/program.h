#pragma once
#include "vulkan/vulkan.hpp"
#include "Shader.h"
#include <vector>
#include <optional>

class GPUProgram final {
public:
	GPUProgram(std::string vertex, std::string fragment);
	GPUProgram(std::string vertex, std::string geometry, std::string fragment);
	~GPUProgram();
	std::optional<Shader> Vertex;
	std::optional<Shader> TessellationControl;
	std::optional<Shader> TessellationEvaluation;
	std::optional<Shader> Geometry;
	std::optional<Shader> Fragment;

	std::vector<vk::PipelineShaderStageCreateInfo> stages;
};