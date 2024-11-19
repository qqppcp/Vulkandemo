#pragma once

#include <vulkan/vulkan.hpp>

enum LoadOp
{
	DontCare = 0,
	Load,
	Clear,
};

struct RenderTarget
{
	std::vector<vk::Format> formats;
	vk::Format depthFormat;
	std::vector<vk::ImageView> views;
	vk::ImageView depthView;
	std::vector<LoadOp> clears;
	uint32_t width;
	uint32_t height;
	int depth = -1;
	int stencil = -1;
};