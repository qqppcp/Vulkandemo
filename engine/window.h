#pragma once
#include <cstdint>
#include "vulkan/vulkan.hpp"

void CreateWindow(uint32_t width, uint32_t height, const char* pTitle, bool hidewindow = false);
bool WindowShouleClose();
void WindowEventProcessing();
void DestroyWindow();
const char** windowGetVulkanExtensions(uint32_t* pExtensionCount);
template<typename T>
void GetWindowSize(T& width, T& height);

std::tuple<uint32_t, uint32_t> GetWindowSize();
VkSurfaceKHR GetVkSurface(vk::Instance instance);
