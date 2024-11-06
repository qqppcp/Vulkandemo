#pragma once

#include <vulkan/vulkan.hpp>
#include <string_view>
#include "Buffer.h"

class TextureManager;

class Texture final {
public:
    friend class TextureManager;
    ~Texture();
    Texture() {}
    vk::Image image;
    vk::DeviceMemory memory;
    vk::ImageView view;

    vk::Format format;
    vk::ImageUsageFlags usageFlag;
    vk::SampleCountFlagBits flags;
    vk::ImageLayout layout;
    bool is_depth;
    bool is_stencil;
    uint32_t width;
    uint32_t height;
    uint32_t depth;

private:
    Texture(std::string_view filename);
    Texture(std::string_view filename, vk::Format format);

    Texture(void* data, uint32_t w, uint32_t h, vk::Format format = vk::Format::eR8G8B8A8Srgb);
    Texture(uint32_t w, uint32_t h, vk::Format format, vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);

    void createImage(uint32_t w, uint32_t h);
    void createImageView();
    void allocMemory();
    uint32_t queryImageMemoryIndex();
    void transitionImageLayoutFromUndefine2Dst(vk::CommandBuffer buffer);
    void transitionImageLayoutFromDst2Optimal(vk::CommandBuffer buffer);
    void transitionImageLayoutFromUndefine2Opt(vk::CommandBuffer buffer);
    void transformData2Image(vk::CommandBuffer cmdbuf, Buffer&, uint32_t w, uint32_t h);

    void init(void* data, uint32_t w, uint32_t h, uint32_t channel, vk::Format format);
};

class TextureManager final {
public:
    static TextureManager& Instance() {
        if (!instance_) {
            instance_.reset(new TextureManager);
        }
        return *instance_;
    }

    std::shared_ptr<Texture> Load(const std::string& filename);
    std::shared_ptr<Texture> LoadHDRCubemap(const std::string& filename, vk::Format format);

    // data must be a RGBA8888 format data
    std::shared_ptr<Texture> Create(void* data, uint32_t w, uint32_t h, vk::Format format = vk::Format::eR8G8B8A8Srgb);
    std::shared_ptr<Texture> Create(uint32_t w, uint32_t h, vk::Format format, vk::ImageUsageFlags usage);
    void Destroy(std::shared_ptr<Texture>);
    void Clear();

private:
    static std::unique_ptr<TextureManager> instance_;

    std::vector<std::shared_ptr<Texture>> datas_;
};