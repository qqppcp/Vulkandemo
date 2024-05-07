#pragma once
#include "vulkan/vulkan.hpp"

struct swapchainCreateParam {
	vk::PhysicalDevice physical_device;
	vk::Device device;
	vk::SurfaceKHR surface;
	vk::SurfaceFormatKHR surfaceFormat;
	vk::PresentModeKHR presentMode;
	uint32_t chainsize; //2��3
	vk::SwapchainKHR oldSwapchain;
};

class Swapchain final {
public:
	vk::SwapchainKHR swapchain;
	Swapchain(uint32_t width, uint32_t height);

	// ��Ҫ��device����ǰ����ʱ����destroy
	~Swapchain();
	void destroy();

	struct SwapchainInfo {
		vk::Extent2D imageExtent;
		uint32_t imageCount;
		vk::SurfaceFormatKHR surfaceFormat;
		vk::SurfaceTransformFlagBitsKHR preTransform;
		vk::PresentModeKHR presentMode;
	};
	SwapchainInfo info;
	std::vector<vk::Image> images;
	std::vector<vk::ImageView> imageviews;
	void queryInfo(uint32_t width, uint32_t height);
};