#pragma once

#include "vulkan/vulkan.hpp"

#include "Swapchain.h"
#include <memory>
#include <optional>

class Texture;

class Context final {
public:
	~Context();
	static void InitContext();
	static void Quit();
	void InitSwapchain();
	void DestroySwapchain();
	static Context& GetInstance();
	vk::Instance instance;
	vk::Queue graphicsQueue;
	vk::Queue presentQueue;
	vk::Queue computeQueue;
	vk::Queue transferQueue;

	std::vector<vk::CommandBuffer> cmdbufs;
	std::shared_ptr<Texture> depth;
	vk::CommandPool graphicsCmdPool;
	vk::SurfaceKHR surface;
	vk::PhysicalDevice physicaldevice;
	vk::Device device;
	std::unique_ptr<Swapchain> swapchain;
	uint32_t current_frame;
	uint32_t image_index;
	struct QueueFamilyIndex {
		std::optional<uint32_t> graphicsFamilyIndex;
		std::optional<uint32_t> presentFamilyIndex;
		std::optional<uint32_t> computeFamilyIndex;
		std::optional<uint32_t> transferFamilyIndex;

		operator bool() {
			return graphicsFamilyIndex.has_value() && presentFamilyIndex.has_value() && computeFamilyIndex.has_value() && transferFamilyIndex.has_value();
		}
	};
	static void enableDefaultFeatures();
	QueueFamilyIndex queueFamileInfo;
private:
	
	static VkPhysicalDeviceVulkan12Features enable12Features_;
	void selectPhysicalDevice();
	void queryQueueFamily(vk::PhysicalDevice);
	void createDevice();
	void destroyDevice();
	Context();
	static std::unique_ptr<Context> instance_;
};