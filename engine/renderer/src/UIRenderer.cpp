#include "UIRenderer.h"
#include "GLFW/glfw3.h"
#include "imgui/imgui.h"
#include "Context.h"
#include "Buffer.h"
#include "window.h"
#include "imgui_impl_vulkan.h"
#include "imgui_impl_glfw.h"

static void check_vk_result(VkResult err)
{
    if (err == 0)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

ImGuiRenderer::ImGuiRenderer()
{
    int maxFlight = Context::GetInstance().swapchain->info.imageCount;
    m_device = Context::GetInstance().device;
    vk::DescriptorPoolSize pool_size;
    pool_size.setDescriptorCount(maxFlight)
        .setType(vk::DescriptorType::eCombinedImageSampler);
    vk::DescriptorPoolCreateInfo poolCI;
    poolCI.setMaxSets(maxFlight)
        .setPoolSizes(pool_size);
    descriptorPool = m_device.createDescriptorPool(poolCI);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Color Schme
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.0f, 0.0f, 0.0f, 0.1f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.8f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.8f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
    style.Colors[ImGuiCol_Button] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);

    // Dimensions
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = 1.0f;
    io.DisplayFramebufferScale = ImVec2(1.f, 1.f);

    // Keymap [GLFW]
    io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
    io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
    io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
    io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
    io.KeyMap[ImGuiKey_Insert] = GLFW_KEY_INSERT;
    io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Space] = GLFW_KEY_SPACE;
    io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
    io.KeyMap[ImGuiKey_KeyPadEnter] = GLFW_KEY_KP_ENTER;
    io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
    io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
    io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
    io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
    io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
    io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(GetWindowHandle(), true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = Context::GetInstance().instance;
    init_info.PhysicalDevice = Context::GetInstance().physicaldevice;
    init_info.Device = Context::GetInstance().device;
    init_info.QueueFamily = Context::GetInstance().queueFamileInfo.graphicsFamilyIndex.value();
    init_info.Queue = Context::GetInstance().graphicsQueue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = descriptorPool;
    init_info.Subpass = 0;
    init_info.MinImageCount = 2;
    init_info.ImageCount = maxFlight;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = check_vk_result;
    //ImGui_ImplVulkan_Init(&init_info, wd->RenderPass);
    //// (this gets a bit more complicated, see example app for full reference)
    //ImGui_ImplVulkan_CreateFontsTexture(YOUR_COMMAND_BUFFER);
    //// (your code submit a queue)
    //ImGui_ImplVulkan_DestroyFontUploadObjects();

    vk::Extent3D fontExtent;

    {
        // Font
        uint8_t* pFontData;
        int imgWidth, imgHeight;
        ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pFontData, &imgWidth, &imgHeight);
        fontExtent = VkExtent3D{ static_cast<uint32_t>(imgWidth), static_cast<uint32_t>(imgHeight), 1 };
        vk::DeviceSize uploadSize = imgWidth * imgHeight * 4;

        // Staging Buffer
        Buffer stagingBuffer(uploadSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        void* pData = m_device.mapMemory(stagingBuffer.memory, 0, uploadSize);
        memcpy(pData, pFontData, uploadSize);
        m_device.unmapMemory(stagingBuffer.memory);

        // Image
        vk::ImageCreateInfo createInfo;
        createInfo.setImageType(vk::ImageType::e2D)
            .setArrayLayers(1)
            .setMipLevels(1)
            .setExtent(fontExtent)
            .setFormat(vk::Format::eR8G8B8A8Unorm)
            .setTiling(vk::ImageTiling::eOptimal)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled)
            .setSamples(vk::SampleCountFlagBits::e1);
        auto m_fontImage = m_device.createImage(createInfo);

        vk::MemoryAllocateInfo allocInfo;

        auto requirements = m_device.getImageMemoryRequirements(m_fontImage);
        allocInfo.setAllocationSize(requirements.size);
        uint32_t index;
        auto properties = Context::GetInstance().physicaldevice.getMemoryProperties();
        for (uint32_t i = 0; i < properties.memoryTypeCount; i++)
        {
            if ((1 << i) & requirements.memoryTypeBits && properties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)
            {
                index = i;
                break;
            }
        }
        allocInfo.setMemoryTypeIndex(index);
        m_memory = m_device.allocateMemory(allocInfo);
        m_device.bindImageMemory(m_fontImage, m_memory, 0);
    }

}

ImGuiRenderer::~ImGuiRenderer()
{
    ImGui::DestroyContext();
    m_device.destroyDescriptorPool(descriptorPool);
}
