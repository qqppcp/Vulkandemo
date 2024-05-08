#pragma once

#include "vulkan/vulkan.hpp"
#include <array>
class Vertex final {
public:

	float x, y;
	float r, g, b, a;
	static std::array<vk::VertexInputAttributeDescription, 2> GetAttribute()
	{
		std::array<vk::VertexInputAttributeDescription, 2> attribs;
		attribs[0].setBinding(0)
			.setLocation(0)
			.setFormat(vk::Format::eR32G32Sfloat)
			.setOffset(0);
		attribs[1].setBinding(0)
			.setLocation(1)
			.setFormat(vk::Format::eR32G32B32A32Sfloat)
			.setOffset(offsetof(Vertex, r));
		return attribs;
	}
	static vk::VertexInputBindingDescription GetBinding()
	{
		vk::VertexInputBindingDescription binding;
		binding.setBinding(0)
			.setInputRate(vk::VertexInputRate::eVertex)
			.setStride(sizeof(Vertex));
		return binding;
	}
};