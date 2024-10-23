#include "Sampler.h"

#include "Context.h"

Sampler::Sampler(vk::Filter minFilter, vk::Filter magFilter,
    vk::SamplerAddressMode addressModeU, vk::SamplerAddressMode addressModeV,
    vk::SamplerAddressMode addressModeW, float maxLod,
    const std::string& name)
{
    device = Context::GetInstance().device;
    vk::SamplerCreateInfo samplerInfo;
    samplerInfo.setMagFilter(magFilter)
        .setMinFilter(minFilter)
        .setMipmapMode(maxLod > 0 ? vk::SamplerMipmapMode::eLinear : vk::SamplerMipmapMode::eNearest)
        .setAddressModeU(addressModeU)
        .setAddressModeV(addressModeV)
        .setAddressModeW(addressModeW)
        .setMipLodBias(0)
        .setAnisotropyEnable(VK_FALSE)
        .setMinLod(0)
        .setMaxLod(maxLod);
    sampler = device.createSampler(samplerInfo);
}

Sampler::Sampler(vk::Filter minFilter, vk::Filter magFilter,
    vk::SamplerAddressMode addressModeU, vk::SamplerAddressMode addressModeV,
    vk::SamplerAddressMode addressModeW, float maxLod, bool compareEnable,
    vk::CompareOp compareOp, const std::string& name)
{
    device = Context::GetInstance().device;
    vk::SamplerCreateInfo samplerInfo;
    samplerInfo.setMagFilter(magFilter)
        .setMinFilter(minFilter)
        .setMipmapMode(maxLod > 0 ? vk::SamplerMipmapMode::eLinear : vk::SamplerMipmapMode::eNearest)
        .setAddressModeU(addressModeU)
        .setAddressModeV(addressModeV)
        .setAddressModeW(addressModeW)
        .setMipLodBias(0)
        .setAnisotropyEnable(VK_FALSE)
        .setCompareEnable(compareEnable)
        .setCompareOp(compareOp)
        .setMinLod(0)
        .setMaxLod(maxLod);
    sampler = device.createSampler(samplerInfo);
}