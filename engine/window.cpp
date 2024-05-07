#include "window.h"
#include "GLFW/glfw3.h"

namespace {
    GLFWwindow* pWindow = nullptr;
    uint32_t _width;
    uint32_t _height;
}

void CreateWindow(uint32_t width, uint32_t height, const char* pTitle, bool hidewindow)
{
    _width = width;
    _height = height;
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, !hidewindow);
    pWindow = glfwCreateWindow(width, height, pTitle, nullptr, nullptr);
}
bool WindowShouleClose()
{
    return glfwWindowShouldClose(pWindow);
}
void WindowEventProcessing()
{
    glfwPollEvents();
}

void DestroyWindow()
{
    glfwDestroyWindow(pWindow);
    pWindow = nullptr;
}

const char** windowGetVulkanExtensions(uint32_t* pExtensionCount)
{
    glfwInit();

    if (!glfwVulkanSupported())
    {
        pExtensionCount = 0;
        return nullptr;
    }

    return glfwGetRequiredInstanceExtensions(pExtensionCount);
}

template<typename T>
inline void GetWindowSize(T& width, T& height)
{
    width = static_cast<T>(_width);
    height = static_cast<T>(_height);
}

std::tuple<uint32_t, uint32_t> GetWindowSize()
{
    return {_width, _height};
}

VkSurfaceKHR GetVkSurface(vk::Instance instance)
{
    VkSurfaceKHR surface;
    glfwCreateWindowSurface(instance, pWindow, nullptr, &surface);
    return surface;
}
