#pragma once
#include "vulkan/vulkan.hpp"


vk::Result CreateDebugCallback(vk::Instance);

vk::Result DestroyDebugCallback(vk::Instance);