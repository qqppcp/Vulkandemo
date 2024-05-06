#pragma once
#include <cstdint>
#include "vulkan\vulkan.hpp"

void CreateWindow(int width, int height, const char* pTitle, bool hidewindow = false);
bool WindowShouleClose();
void WindowEventProcessing();
void DestroyWindow();
const char** windowGetVulkanExtensions(uint32_t* pExtensionCount);

VkSurfaceKHR GetVkSurface(vk::Instance instance);