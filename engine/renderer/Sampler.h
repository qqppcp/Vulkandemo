#pragma once

#include <string>
#include <vulkan/vulkan.hpp>

class Sampler final
{
public:
    explicit Sampler(vk::Filter minFilter, vk::Filter magFilter,
        vk::SamplerAddressMode addressModeU, vk::SamplerAddressMode addressModeV,
        vk::SamplerAddressMode addressModeW, float maxLod,
        const std::string& name = "");

    explicit Sampler(vk::Filter minFilter, vk::Filter magFilter,
        vk::SamplerAddressMode addressModeU, vk::SamplerAddressMode addressModeV,
        vk::SamplerAddressMode addressModeW, float maxLod, bool compareEnable,
        vk::CompareOp compareOp, const std::string& name = "");

    ~Sampler() { device.destroySampler(sampler); }

    vk::Sampler vkSampler() const { return sampler; }

private:
    vk::Device device = VK_NULL_HANDLE;
    vk::Sampler sampler = VK_NULL_HANDLE;
};