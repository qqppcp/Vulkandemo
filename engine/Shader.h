#pragma once
#include "vulkan/vulkan.hpp"
#include <string>

struct Shader
{
public:
	Shader(const std::string& filename);
	~Shader();
	void destroy();
	std::string name;
	vk::ShaderModule module;
};