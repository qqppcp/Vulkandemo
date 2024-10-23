#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "vulkan/vulkan.hpp"
#include "ImGuiBase.h"
#include "imgui.h"
class Uniform : public ImGuiBase {
public:
	glm::vec4 color;
	bool isOpen{ true };
	static vk::DescriptorSetLayoutBinding GetBinding()
	{
		vk::DescriptorSetLayoutBinding binding;
		binding.setBinding(0)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setStageFlags(vk::ShaderStageFlagBits::eFragment)
			.setDescriptorCount(1);
		return binding;
	}

	virtual void customUI() final 
	{
		if (!isOpen)
		{
			return;
		}
		ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar);
		ImGui::Begin("Settings", &isOpen);
		ImGui::ColorEdit3("Triangle Color", glm::value_ptr(color));
		ImGui::End();
	};
};
