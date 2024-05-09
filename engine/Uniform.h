#pragma once

#include "glm/glm.hpp"
#include "vulkan/vulkan.hpp"

class Uniform {
public:
	glm::vec4 color;

	static vk::DescriptorSetLayoutBinding GetBinding()
	{
		vk::DescriptorSetLayoutBinding binding;
		binding.setBinding(0)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setStageFlags(vk::ShaderStageFlagBits::eFragment)
			.setDescriptorCount(1);
		return binding;
	}
};
