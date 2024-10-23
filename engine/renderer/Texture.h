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
    vk::SampleCountFlagBits flags;
    vk::ImageLayout layout;
    bool is_depth;
    bool is_stencil;

private:
    Texture(std::string_view filename);

    Texture(void* data, uint32_t w, uint32_t h);
    Texture(uint32_t w, uint32_t h, vk::Format format);

    void createImage(uint32_t w, uint32_t h);
    void createImageView();
    void allocMemory();
    uint32_t queryImageMemoryIndex();
    void transitionImageLayoutFromUndefine2Dst(vk::CommandBuffer buffer);
    void transitionImageLayoutFromDst2Optimal(vk::CommandBuffer buffer);
    void transitionImageLayoutFromUndefine2Opt(vk::CommandBuffer buffer);
    void transformData2Image(vk::CommandBuffer cmdbuf, Buffer&, uint32_t w, uint32_t h);

    void init(void* data, uint32_t w, uint32_t h);
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

    // data must be a RGBA8888 format data
    std::shared_ptr<Texture> Create(void* data, uint32_t w, uint32_t h);
    std::shared_ptr<Texture> Create(uint32_t w, uint32_t h, vk::Format format);
    void Destroy(std::shared_ptr<Texture>);
    void Clear();

private:
    static std::unique_ptr<TextureManager> instance_;

    std::vector<std::shared_ptr<Texture>> datas_;
};