#include "window.h"
#include "GLFW/glfw3.h"

namespace {
    GLFWwindow* pWindow = nullptr;
}

void CreateWindow(int width, int height, const char* pTitle, bool hidewindow)
{
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


VkSurfaceKHR GetVkSurface(vk::Instance instance)
{
    VkSurfaceKHR surface;
    glfwCreateWindowSurface(instance, pWindow, nullptr, &surface);
    return surface;
}
