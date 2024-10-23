#include "window.h"
#include "GLFW/glfw3.h"
#include "input.h"

ImGuiTermination* termination = nullptr;

namespace {
    GLFWwindow* pWindow = nullptr;
    uint32_t _width;
    uint32_t _height;
}

void keyCallback(GLFWwindow*, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        InputManager::GetInstance().ProcessKey((KEY)key, true);
    }
    else if (action == GLFW_RELEASE)
    {
        InputManager::GetInstance().ProcessKey((KEY)key, false);
    }
}
void positionCallback(GLFWwindow*, double xPos, double yPos)
{
    InputManager::GetInstance().ProcessMouseMove(xPos, yPos);
}
void scrollCallback(GLFWwindow*, double xOffset, double yOffset)
{
    InputManager::GetInstance().ProcessMouseWheel(yOffset);
}
void buttonCallback(GLFWwindow*, int button, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        InputManager::GetInstance().ProcessButton((BUTTON)button, true);
    }
    else if (action == GLFW_RELEASE)
    {
        InputManager::GetInstance().ProcessButton((BUTTON)button, false);
    }
}

void CreateWindow(uint32_t width, uint32_t height, const char* pTitle, bool hidewindow)
{
    _width = width;
    _height = height;
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, !hidewindow);
    pWindow = glfwCreateWindow(width, height, pTitle, nullptr, nullptr);
    termination = new ImGuiTermination;
    glfwSetKeyCallback(pWindow, keyCallback);
    glfwSetCursorPosCallback(pWindow, positionCallback);
    glfwSetScrollCallback(pWindow, scrollCallback);
    glfwSetMouseButtonCallback(pWindow, buttonCallback);
}
bool WindowShouleClose()
{
    return glfwWindowShouldClose(pWindow) || termination->terminationRequested();
}
void WindowEventProcessing()
{
    glfwPollEvents();
}

void DestroyWindow()
{
    delete termination;
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

GLFWwindow* GetWindowHandle()
{
    return pWindow;
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

ImGuiTermination* GetTermination()
{
    return termination;
}
