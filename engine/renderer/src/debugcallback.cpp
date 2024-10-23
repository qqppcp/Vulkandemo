#include "debugcallback.h"
#include "log.h"

namespace {
	vk::DebugReportCallbackEXT callback{};
	VkBool32 MessageCallback(VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objectType,
		uint64_t object,
		size_t location,
		int32_t messageCode,
		const char* pLayerPrefix,
		const char* pMessage,
		void* pUserData)
	{
		// TODO: log system
		if ((flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) != 0) 
			DEMO_LOG(Error, std::format("[{}] Code {} : {}", pLayerPrefix, messageCode, pMessage));
		if ((flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) != 0 ||
			(flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) != 0) 
			DEMO_LOG(Warning, std::format("[{}] Code {} : {}", pLayerPrefix, messageCode, pMessage));
		if ((flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) != 0)
			DEMO_LOG(Info, std::format("[{}] Code {} : {}", pLayerPrefix, messageCode, pMessage));
		if ((flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) != 0) 
			DEMO_LOG(Debug, std::format("[{}] Code {} : {}", pLayerPrefix, messageCode, pMessage));

		return VK_FALSE;
	}

}
static bool initialized = false;
vk::Result CreateDebugCallback(vk::Instance instance)
{
	vk::DebugReportCallbackCreateInfoEXT debugCallbackCI;
	debugCallbackCI.setFlags(vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning |
		vk::DebugReportFlagBitsEXT::ePerformanceWarning | vk::DebugReportFlagBitsEXT::eInformation);
	debugCallbackCI.setPfnCallback(MessageCallback);
	VkDebugReportCallbackCreateInfoEXT debugReportCallbackCI{ .sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
		.pNext = nullptr,
		.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT |
	VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_INFORMATION_BIT_EXT ,
	.pfnCallback = &MessageCallback};
	//∂ØÃ¨º”‘ÿ
	PFN_vkGetInstanceProcAddr instance_proc = reinterpret_cast<PFN_vkGetInstanceProcAddr>(instance.getProcAddr("vkGetInstanceProcAddr"));
	vk::DispatchLoaderDynamic dld(instance, instance_proc);
	if (!dld.vkCreateDebugReportCallbackEXT)
		DEMO_LOG(Error, "NULL HANDLE");

	callback = instance.createDebugReportCallbackEXT(debugCallbackCI, nullptr, dld);

	initialized = true;
	return vk::Result::eSuccess;
}

vk::Result DestroyDebugCallback(vk::Instance instance)
{

	if (!initialized)
		return vk::Result::eErrorInitializationFailed;
	auto instance_proc = reinterpret_cast<PFN_vkGetInstanceProcAddr>(instance.getProcAddr("vkGetInstanceProcAddr"));
	vk::DispatchLoaderDynamic dld(instance, instance_proc);
	instance.destroyDebugReportCallbackEXT(callback, nullptr, dld);
	return vk::Result::eSuccess;
}
