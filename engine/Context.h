#pragma once

#include "vulkan/vulkan.hpp"

#include <memory>
#include <optional>

class Context final {
public:
	~Context();
	static void InitContext();
	static Context& GetInstance();
	vk::Instance instance;
	vk::Queue graphicsQueue;
	vk::Queue presentQueue;
	vk::Queue computeQueue;
	vk::Queue transferQueue;
private:
	vk::PhysicalDevice phydevice;
	vk::SurfaceKHR surface;
	vk::PhysicalDevice physicaldevice;
	vk::Device device;
	struct QueueFamilyIndex {
		std::optional<uint32_t> graphicsFamilyIndex;
		std::optional<uint32_t> presentFamilyIndex;
		std::optional<uint32_t> computeFamilyIndex;
		std::optional<uint32_t> transferFamilyIndex;

		operator bool() {
			return graphicsFamilyIndex.has_value() && presentFamilyIndex.has_value() && computeFamilyIndex.has_value() && transferFamilyIndex.has_value();
		}
	};
	
	void selectPhysicalDevice();
	void queryQueueFamily(vk::PhysicalDevice);
	void createDevice();
	void destroyDevice();
	QueueFamilyIndex queueFamileInfo;
	Context();
	static Context* instance_;
};