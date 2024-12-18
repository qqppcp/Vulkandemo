#include "Context.h"
#include "window.h"
#include "debugcallback.h"
#include "log.h"
#include "Texture.h"
#include "CommandBuffer.h"

#include <string>
#include <format>
#include <unordered_set>
#include <any>

std::unique_ptr<Context> Context::instance_ = nullptr;
bool validation = true;

VkPhysicalDeviceVulkan12Features Context::enable12Features_ = {
	.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
};

Context::~Context()
{
	device.destroyCommandPool(graphicsCmdPool);
	instance.destroySurfaceKHR(surface);
	destroyDevice();
#ifdef _DEBUG
	DestroyDebugCallback(instance);
#endif // _DEBUG
	instance.destroy();
}

void Context::InitContext()
{
	if (!instance_)
		instance_.reset(new Context());

}

void Context::Quit()
{
	instance_.reset();
}

void Context::InitSwapchain()
{
	auto [width, height] = GetWindowSize();
	swapchain.reset(new Swapchain(width, height));
	depth = TextureManager::Instance().Create(width, height, vk::Format::eD24UnormS8Uint, vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled);
	uint32_t maxFlight = Context::GetInstance().swapchain->info.imageCount;
	cmdbufs = CommandManager::Allocate(graphicsCmdPool, maxFlight, true);
}

void Context::DestroySwapchain()
{
	CommandManager::Free(graphicsCmdPool, cmdbufs);
	TextureManager::Instance().Destroy(depth);
	depth.reset();
	swapchain.reset();
}
Context& Context::GetInstance()
{
	if (!instance_)
		throw std::runtime_error("Can't get the handle before Init");
	return *instance_;
}

Context::Context()
{
	std::vector<const char*> layers;
	std::vector<const char*> extensions;
	if (validation)
		layers.emplace_back("VK_LAYER_KHRONOS_validation");

	uint32_t extensionCount;
	const char** extensionNames = windowGetVulkanExtensions(&extensionCount);
	for (uint32_t i = 0; i < extensionCount; ++i)
		extensions.emplace_back(extensionNames[i]);
	
#ifdef _DEBUG
	extensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif // _DEBUG

	vk::InstanceCreateInfo createinfo;
	vk::ApplicationInfo appinfo;
	appinfo.setApiVersion(VK_API_VERSION_1_3)
		.setPApplicationName("demo");
	createinfo.setPApplicationInfo(&appinfo)
		.setPEnabledLayerNames(layers)
		.setPEnabledExtensionNames(extensions);
	instance = vk::createInstance(createinfo);
	DEMO_LOG(Info, "success to create vulkan instance.");

#ifdef _DEBUG
	CreateDebugCallback(instance);
#endif // _DEBUG

	surface = GetVkSurface(instance);
	selectPhysicalDevice();
	createDevice();
	vk::CommandPoolCreateInfo poolCI;
	poolCI.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
		.setQueueFamilyIndex(queueFamileInfo.graphicsFamilyIndex.value());
	graphicsCmdPool = device.createCommandPool(poolCI);
	current_frame = 0;
}


void Context::enableDefaultFeatures()
{
	// do we need these for defaults?
	enable12Features_.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
	enable12Features_.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;
	// enable12Features_.descriptorBindingUniformBufferUpdateAfterBind =
	//     VK_TRUE,  // This
	//               // makes creating a device on a 1060 fail
	enable12Features_.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
	enable12Features_.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
	enable12Features_.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
	enable12Features_.descriptorBindingPartiallyBound = VK_TRUE;
	enable12Features_.descriptorBindingVariableDescriptorCount = VK_TRUE;
	enable12Features_.descriptorIndexing = VK_TRUE;
	enable12Features_.runtimeDescriptorArray = VK_TRUE;
}

void Context::selectPhysicalDevice()
{
	auto phydevices = instance.enumeratePhysicalDevices();

	for (auto phydevice : phydevices)
	{
		vk::PhysicalDeviceProperties properties = phydevice.getProperties();
		vk::PhysicalDeviceFeatures features = phydevice.getFeatures();
		vk::PhysicalDeviceMemoryProperties memory = phydevice.getMemoryProperties();
		DEMO_LOG(Info, std::format("Evaluating device: {}", (char*)properties.deviceName));
		if (properties.deviceType != vk::PhysicalDeviceType::eDiscreteGpu)
		{
			DEMO_LOG(Info, "Device is not a discrete GPU. Skipping.");
			continue;
		}
		queryQueueFamily(phydevice);
		if (queueFamileInfo)
		{
			DEMO_LOG(Info, "Graphics | Present | Compute | Transfer | Name");
			DEMO_LOG(Info, std::format("       {} |       {} |       {} |        {} | {}",
				queueFamileInfo.graphicsFamilyIndex.value(),
				queueFamileInfo.presentFamilyIndex.value(),
				queueFamileInfo.computeFamilyIndex.value(),
				queueFamileInfo.transferFamilyIndex.value(),
				(char*)properties.deviceName));
			this->physicaldevice = phydevice;
			DEMO_LOG(Info, std::format("Select device: {}", (char*)properties.deviceName));
			switch (properties.deviceType)
			{
			default:
			case vk::PhysicalDeviceType::eOther:
				DEMO_LOG(Info, "GPU type is Unknown."); break;
			case vk::PhysicalDeviceType::eIntegratedGpu:
				DEMO_LOG(Info, "GPU type is Integrated."); break;
			case vk::PhysicalDeviceType::eDiscreteGpu:
				DEMO_LOG(Info, "GPU type is Discrete."); break;
			case vk::PhysicalDeviceType::eVirtualGpu:
				DEMO_LOG(Info, "GPU type is Virtual."); break;
			case vk::PhysicalDeviceType::eCpu:
				DEMO_LOG(Info, "GPU type is CPU."); break;
			}
			DEMO_LOG(Info, std::format("GPU Driver version: {}.{}.{}",
				VK_VERSION_MAJOR(properties.driverVersion),
				VK_VERSION_MINOR(properties.driverVersion),
				VK_VERSION_PATCH(properties.driverVersion)));
			for (uint32_t i = 0; i < memory.memoryHeapCount; ++i)
			{
				float memory_size_gib = (((float)memory.memoryHeaps[i].size) / 1024.0f / 1024.0f / 1024.0f);
				if (memory.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal)
				{
					DEMO_LOG(Info, std::format("Local GPU memory: {:.2f} GiB", memory_size_gib));
				}
				else
				{
					DEMO_LOG(Info, std::format("Shared System memory: {:.2f} GiB", memory_size_gib));
				}
			}
			break;
		}
	}
}

void Context::queryQueueFamily(vk::PhysicalDevice physicalDevice)
{
	auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
	for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i)
	{
		if (!queueFamileInfo.graphicsFamilyIndex.has_value() && queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics)
		{
			queueFamileInfo.graphicsFamilyIndex = i;
		}
		if (!queueFamileInfo.presentFamilyIndex.has_value() && physicalDevice.getSurfaceSupportKHR(i, surface))
		{
			queueFamileInfo.presentFamilyIndex = i;
		}
		if (!queueFamileInfo.computeFamilyIndex.has_value() && queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eCompute)
		{
			queueFamileInfo.computeFamilyIndex = i;
		}
		if (!queueFamileInfo.transferFamilyIndex.has_value() && queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eTransfer)
		{
			queueFamileInfo.transferFamilyIndex = i;
		}
		if (queueFamileInfo)
		{
			return;
		}
	}
}

void Context::createDevice() {
	std::vector<const char*> extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME };
	vk::PhysicalDeviceFeatures features;
	features.setSamplerAnisotropy(true)
		.setFillModeNonSolid(true)
		.setDrawIndirectFirstInstance(true)
		.setMultiDrawIndirect(true);
	std::unordered_set<uint32_t> uniqueIndex;
	bool shared[3] = { 0,0,0 };
	uniqueIndex.insert(queueFamileInfo.graphicsFamilyIndex.value());
	if (uniqueIndex.find(queueFamileInfo.presentFamilyIndex.value()) != uniqueIndex.end())
		shared[0] = true;
	else
		uniqueIndex.insert(queueFamileInfo.presentFamilyIndex.value());

	if (uniqueIndex.find(queueFamileInfo.computeFamilyIndex.value()) != uniqueIndex.end())
		shared[1] = true;
	else
		uniqueIndex.insert(queueFamileInfo.computeFamilyIndex.value());

	if (uniqueIndex.find(queueFamileInfo.transferFamilyIndex.value()) != uniqueIndex.end())
		shared[2] = true;
	else
		uniqueIndex.insert(queueFamileInfo.transferFamilyIndex.value());
	std::vector<vk::DeviceQueueCreateInfo> queueCIs;
	float priorities = 1.0f;
	vk::DeviceQueueCreateInfo queueCI;
	queueCI.setPQueuePriorities(&priorities)
		.setQueueCount(1)
		.setQueueFamilyIndex(queueFamileInfo.graphicsFamilyIndex.value());
	queueCIs.emplace_back(queueCI);
	if (!shared[0])
	{
		queueCI.setPQueuePriorities(&priorities)
			.setQueueCount(1)
			.setQueueFamilyIndex(queueFamileInfo.presentFamilyIndex.value());
		queueCIs.emplace_back(queueCI);
	}if (!shared[1])
	{
		queueCI.setPQueuePriorities(&priorities)
			.setQueueCount(1)
			.setQueueFamilyIndex(queueFamileInfo.computeFamilyIndex.value());
		queueCIs.emplace_back(queueCI);
	}
	if (!shared[2])
	{
		queueCI.setPQueuePriorities(&priorities)
			.setQueueCount(1)
			.setQueueFamilyIndex(queueFamileInfo.transferFamilyIndex.value());
		queueCIs.emplace_back(queueCI);
	}

	vk::PhysicalDeviceShaderDrawParametersFeatures shaderDrawParametersFeatures;
	shaderDrawParametersFeatures.setShaderDrawParameters(VK_TRUE);

	vk::PhysicalDeviceVulkan12Features vulkan12Features;
	vulkan12Features.setDrawIndirectCount(true)
		.setBufferDeviceAddressCaptureReplay(true)
		.setShaderSampledImageArrayNonUniformIndexing(true)
		.setShaderStorageImageArrayNonUniformIndexing(true)
		.setDescriptorBindingSampledImageUpdateAfterBind(true)
		.setDescriptorBindingStorageBufferUpdateAfterBind(true)
		.setDescriptorBindingUpdateUnusedWhilePending(true)
		.setDescriptorBindingPartiallyBound(true)
		.setDescriptorBindingVariableDescriptorCount(true)
		.setRuntimeDescriptorArray(true)
		.setBufferDeviceAddress(true)
		.setBufferDeviceAddressCaptureReplay(true)
		.setPNext(&shaderDrawParametersFeatures);
	vk::PhysicalDeviceFeatures2 features2;
	features2.setPNext(&vulkan12Features)
		.setFeatures(features);
	vk::DeviceCreateInfo deviceCI;
	deviceCI.setQueueCreateInfos(queueCIs)
		.setPEnabledExtensionNames(extensions)
		//.setPEnabledFeatures(&features)
		.setPNext(&features2);
	device = physicaldevice.createDevice(deviceCI);

	graphicsQueue = device.getQueue(queueFamileInfo.graphicsFamilyIndex.value(), 0);
	presentQueue = device.getQueue(queueFamileInfo.presentFamilyIndex.value(), 0);
	computeQueue = device.getQueue(queueFamileInfo.computeFamilyIndex.value(), 0);
	transferQueue = device.getQueue(queueFamileInfo.transferFamilyIndex.value(), 0);
}

void Context::destroyDevice()
{
	device.destroy();
}
