#include "Swapchain.h"
#include "Context.h"

namespace {
	bool valid = false;
}

void Swapchain::queryInfo(uint32_t width, uint32_t height)
{
	vk::PhysicalDevice physical_device = Context::GetInstance().physicaldevice;
	vk::SurfaceKHR surface = Context::GetInstance().surface;
	vk::SurfaceCapabilitiesKHR capabilities = physical_device.getSurfaceCapabilitiesKHR(surface);
	info.imageCount = std::clamp<uint32_t>(3, capabilities.minImageCount, capabilities.maxImageCount);
	info.imageExtent.width = std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	info.imageExtent.height = std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
	info.preTransform = capabilities.currentTransform;

	auto formats = physical_device.getSurfaceFormatsKHR(surface);
	info.surfaceFormat = formats[0];
	for (auto format : formats)
	{
		if (format.format == vk::Format::eR8G8B8A8Srgb &&
			format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
		{
			info.surfaceFormat = format;
			break;
		}
	}
	auto presents = physical_device.getSurfacePresentModesKHR(surface);
	info.presentMode = vk::PresentModeKHR::eFifo;
	for (auto present : presents)
	{
		if (present == vk::PresentModeKHR::eMailbox)
		{
			info.presentMode = present;
			break;
		}
	}
}
Swapchain::Swapchain(uint32_t width, uint32_t height)
{
	vk::Device device = Context::GetInstance().device;
	vk::SurfaceKHR surface = Context::GetInstance().surface;
	queryInfo(width, height);
	vk::SwapchainCreateInfoKHR swapchainCI;
	swapchainCI.setSurface(surface)
		.setClipped(true)
		.setImageArrayLayers(1)
		.setMinImageCount(info.imageCount)
		.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
		.setImageColorSpace(info.surfaceFormat.colorSpace)
		.setImageFormat(info.surfaceFormat.format)
		.setImageExtent(info.imageExtent)
		.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
		.setPresentMode(info.presentMode);
	auto queueIndices = Context::GetInstance().queueFamileInfo;
	if (queueIndices.graphicsFamilyIndex.value() == queueIndices.presentFamilyIndex.value())
	{
		swapchainCI.setQueueFamilyIndices(queueIndices.graphicsFamilyIndex.value())
			.setImageSharingMode(vk::SharingMode::eExclusive);
	}
	else
	{
		std::array indices = { queueIndices.graphicsFamilyIndex.value() , queueIndices.presentFamilyIndex.value() };
		swapchainCI.setQueueFamilyIndices(indices)
			.setImageSharingMode(vk::SharingMode::eConcurrent);
	}
	swapchain = device.createSwapchainKHR(swapchainCI);
	valid = true;

	images = device.getSwapchainImagesKHR(swapchain);
	imageviews.resize(images.size());
	for (size_t i = 0; i < images.size(); i++)
	{
		vk::ImageViewCreateInfo viewCI;
		vk::ComponentMapping mapping;
		vk::ImageSubresourceRange range;
		range.setLayerCount(1)
			.setLevelCount(1)
			.setBaseArrayLayer(0)
			.setBaseMipLevel(0)
			.setAspectMask(vk::ImageAspectFlagBits::eColor);
		viewCI.setImage(images[i])
			.setComponents(mapping)
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(info.surfaceFormat.format)
			.setSubresourceRange(range);
		imageviews[i] = device.createImageView(viewCI);
	}
}

Swapchain::~Swapchain()
{
	if (valid)
	{
		destroy();
	}
}

void Swapchain::destroy()
{
	for (auto view : imageviews)
		Context::GetInstance().device.destroyImageView(view);
	
	Context::GetInstance().device.destroySwapchainKHR(swapchain);
	valid = false;
}
