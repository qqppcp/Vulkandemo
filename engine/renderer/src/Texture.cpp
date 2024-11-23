#include "Texture.h"
#include <stb_image.h>
#include <format>

#include "log.h"
#include "Context.h"
#include "CommandBuffer.h"
#include "convert2Cubemap.h"

namespace
{
    std::string extFormat = ".hdr";
}

Texture::Texture(std::string_view filename) {
    stbi_set_flip_vertically_on_load(true);
    int w, h, channel;
    stbi_uc* pixels = stbi_load(filename.data(), &w, &h, &channel, STBI_rgb_alpha);
    size_t size = w * h * 4;

    if (!pixels) {
        throw std::runtime_error("image load failed");
    }

    init(pixels, w, h, 4, vk::Format::eR8G8B8A8Srgb);

    stbi_image_free(pixels);
}

Texture::Texture(std::string_view filename, vk::Format format)
{
    stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;
    float* data = stbi_loadf(filename.data(), &width, &height, &nrComponents, STBI_rgb_alpha);
    if (!data)
    {
        throw std::runtime_error("image load failed");
    }
    vk::ImageFormatProperties formatProperties;
    vk::PhysicalDeviceImageFormatInfo2 formatInfo;
    formatInfo.setFormat(vk::Format::eR32G32B32A32Sfloat)
        .setType(vk::ImageType::e2D)
        .setTiling(vk::ImageTiling::eOptimal)
        .setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);
    vk::ImageFormatProperties2 properties;
    auto result = Context::GetInstance().physicaldevice.getImageFormatProperties2(&formatInfo, &properties);

    std::shared_ptr<Texture> temp;
    temp.reset(new Texture());
    temp->init(data, width, height, 4 * sizeof(float), vk::Format::eR32G32B32A32Sfloat);
    stbi_image_free(data);
    auto ret = convert2Cubemap(temp); 
    this->image = ret->image;
    this->view = ret->view;
    this->memory = ret->memory;
    ret->image = VK_NULL_HANDLE;
    ret->view = VK_NULL_HANDLE;
    ret->memory = VK_NULL_HANDLE;
    temp.reset();
    ret.reset();
}

Texture::Texture(void* data, unsigned int w, unsigned int h, vk::Format format) {
    init(data, w, h, 4, format);
}

Texture::Texture(void* data, unsigned int w, unsigned int h, unsigned int channel, vk::Format format) {
    init(data, w, h, channel, format);
}

Texture::Texture(uint32_t w, uint32_t h, vk::Format format, vk::ImageUsageFlags usage, int miplevels)
{
    width = w;
    height = h;
    this->format = format;
    this->miplevels = miplevels;
    usageFlag = usage;
    flags = vk::SampleCountFlagBits::e1;
    if (format == vk::Format::eD24UnormS8Uint || format == vk::Format::eD16Unorm || format == vk::Format::eD16UnormS8Uint
        || format == vk::Format::eD32Sfloat || format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eX8D24UnormPack32)
    {
        is_depth = true;
        if (format == vk::Format::eD16UnormS8Uint || format == vk::Format::eD24UnormS8Uint
            || format == vk::Format::eD32SfloatS8Uint)
        {
            is_stencil = true;
            layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        }
        else
        {
            is_stencil = false;
            layout = vk::ImageLayout::eDepthAttachmentOptimal;
        }
    }
    else if (format == vk::Format::eS8Uint)
    {
        is_depth = false;
        is_stencil = true;
        layout = vk::ImageLayout::eStencilAttachmentOptimal;
    }
    else
    {
        is_depth = false;
        is_stencil = false;
        layout = vk::ImageLayout::eColorAttachmentOptimal;
    }
    if (format == vk::Format::eR32G32Sfloat)
    {
        layout = vk::ImageLayout::eUndefined;
    }
    auto& device = Context::GetInstance().device;
    vk::ImageCreateInfo createInfo;
    createInfo.setImageType(vk::ImageType::e2D)
        .setSharingMode(vk::SharingMode::eExclusive)
        .setArrayLayers(1)
        .setMipLevels(miplevels)
        .setExtent({ w, h, 1 })
        .setFormat(format)
        .setTiling(vk::ImageTiling::eOptimal)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setUsage(usageFlag)
        .setSamples(vk::SampleCountFlagBits::e1);
    image = device.createImage(createInfo);
    vk::MemoryAllocateInfo allocInfo;

    auto requirements = device.getImageMemoryRequirements(image);
    allocInfo.setAllocationSize(requirements.size);

    uint32_t index = 0;
    auto properties = Context::GetInstance().physicaldevice.getMemoryProperties();
    for (size_t i = 0; i < properties.memoryTypeCount; i++)
    {
        if ((1 << i) & requirements.memoryTypeBits && properties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)
        {
            index = i;
            break;
        }
    }
    allocInfo.setMemoryTypeIndex(index);
    memory = device.allocateMemory(allocInfo);
    device.bindImageMemory(image, memory, 0);
    auto cmdbuf = CommandManager::BeginSingle(Context::GetInstance().graphicsCmdPool);
    //transitionImageLayoutFromUndefine2Opt(cmdbuf);
    CommandManager::EndSingle(Context::GetInstance().graphicsCmdPool, cmdbuf, Context::GetInstance().graphicsQueue);
    vk::ImageViewCreateInfo viewCreateInfo;
    vk::ComponentMapping mapping;
    vk::ImageSubresourceRange range;
    mapping.setR(vk::ComponentSwizzle::eIdentity)
        .setG(vk::ComponentSwizzle::eIdentity)
        .setB(vk::ComponentSwizzle::eIdentity)
        .setA(vk::ComponentSwizzle::eIdentity);
    range.setAspectMask((is_depth ? vk::ImageAspectFlagBits::eDepth : (is_stencil ? vk::ImageAspectFlagBits::eStencil :
            vk::ImageAspectFlagBits::eColor)))
        .setBaseArrayLayer(0)
        .setLayerCount(1)
        .setLevelCount(miplevels)
        .setBaseMipLevel(0);
    viewCreateInfo.setImage(image)
        .setViewType(vk::ImageViewType::e2D)
        .setComponents(mapping)
        .setFormat(format)
        .setSubresourceRange(range);
    view = Context::GetInstance().device.createImageView(viewCreateInfo);
}

void Texture::init(void* data, uint32_t w, uint32_t h, uint32_t channel, vk::Format format) {
    width = w;
    height = h;
    this->format = format;
    flags = vk::SampleCountFlagBits::e1;
    layout = vk::ImageLayout::eShaderReadOnlyOptimal;
    is_depth = false;
    is_stencil = false;
    const uint32_t size = w * h * channel;
    std::unique_ptr<Buffer> buffer(new Buffer(size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible));

    auto device = Context::GetInstance().device;
    void* p = device.mapMemory(buffer->memory, 0, size);
    memcpy(p, data, size);
    device.unmapMemory(buffer->memory);

    createImage(w, h);
    allocMemory();
    device.bindImageMemory(image, memory, 0);

    auto cmdbuf = CommandManager::BeginSingle(Context::GetInstance().graphicsCmdPool);

    transitionImageLayoutFromUndefine2Dst(cmdbuf);
    transformData2Image(cmdbuf, *buffer, w, h);
    transitionImageLayoutFromDst2Optimal(cmdbuf);
    CommandManager::EndSingle(Context::GetInstance().graphicsCmdPool, cmdbuf, Context::GetInstance().graphicsQueue);

    createImageView();
}

Texture::~Texture() {
    auto& device = Context::GetInstance().device;
    device.destroyImageView(view);
    device.freeMemory(memory);
    device.destroyImage(image);
}

void Texture::transitionImageLayout(vk::CommandBuffer cmdbuf, vk::ImageLayout newLayout)
{
    vk::AccessFlags srcAccessMask = vk::AccessFlagBits::eNone;
    vk::AccessFlags dstAccessMask = vk::AccessFlagBits::eNone;
    vk::PipelineStageFlags sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
    vk::PipelineStageFlags destinationStage = vk::PipelineStageFlagBits::eBottomOfPipe;

    vk::PipelineStageFlags depthStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests |
        vk::PipelineStageFlagBits::eLateFragmentTests;
    vk::PipelineStageFlags sampledStageMask = vk::PipelineStageFlagBits::eVertexShader |
        vk::PipelineStageFlagBits::eFragmentShader | vk::PipelineStageFlagBits::eComputeShader;

    auto oldLayout = layout;
    if (oldLayout == newLayout)
    {
        return;
    }

    switch (oldLayout)
    {
    case vk::ImageLayout::eUndefined:
        break;
    case vk::ImageLayout::eGeneral:
        sourceStage = vk::PipelineStageFlagBits::eAllCommands;
        srcAccessMask = vk::AccessFlagBits::eMemoryWrite;
        break;
    case vk::ImageLayout::eColorAttachmentOptimal:
        sourceStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        break;
    case vk::ImageLayout::eDepthStencilAttachmentOptimal:
        sourceStage = depthStageMask;
        srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        break;
    case vk::ImageLayout::eDepthStencilReadOnlyOptimal:
        sourceStage = depthStageMask | sampledStageMask;
        break;
    case vk::ImageLayout::eShaderReadOnlyOptimal:
        sourceStage = sampledStageMask;
        break;
    case vk::ImageLayout::eTransferSrcOptimal:
        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        break;
    case vk::ImageLayout::eTransferDstOptimal:
        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        break;
    case vk::ImageLayout::ePreinitialized:
        sourceStage = vk::PipelineStageFlagBits::eHost;
        srcAccessMask = vk::AccessFlagBits::eHostWrite;
        break;
    case vk::ImageLayout::ePresentSrcKHR:
        break;
    default:
        DEMO_LOG(Error, "Unknown image layout");
        break;
    }

    switch (newLayout)
    {
    case vk::ImageLayout::eGeneral:
    case vk::ImageLayout::eFragmentDensityMapOptimalEXT:
        destinationStage = vk::PipelineStageFlagBits::eAllCommands;
        dstAccessMask = vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eMemoryWrite;
        break;
    case vk::ImageLayout::eColorAttachmentOptimal:
        destinationStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
        break;
    case vk::ImageLayout::eDepthStencilAttachmentOptimal:
        destinationStage = depthStageMask;
        dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | 
            vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        break;
    case vk::ImageLayout::eDepthStencilReadOnlyOptimal:
        destinationStage = depthStageMask | sampledStageMask;
        dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | 
            vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eInputAttachmentRead;
        break;
    case vk::ImageLayout::eShaderReadOnlyOptimal:
        destinationStage = sampledStageMask;
        dstAccessMask = vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eInputAttachmentRead;
        break;
    case vk::ImageLayout::eTransferSrcOptimal:
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
        dstAccessMask = vk::AccessFlagBits::eTransferRead;
        break;
    case vk::ImageLayout::eTransferDstOptimal:
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
        dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        break;
    case vk::ImageLayout::ePresentSrcKHR:
        break;
    default:
        DEMO_LOG(Error, "Unknown image layout");
        break;
    }
    vk::ImageAspectFlags aspectMask = is_depth ? is_stencil ? vk::ImageAspectFlagBits::eStencil | vk::ImageAspectFlagBits::eDepth :
        vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
    vk::ImageSubresourceRange range;
    range.setAspectMask(aspectMask)
        .setBaseArrayLayer(0)
        .setBaseMipLevel(0)
        .setLayerCount(1)
        .setLevelCount(miplevels);
    vk::ImageMemoryBarrier barrier;
    barrier.setSrcAccessMask(srcAccessMask)
        .setDstAccessMask(dstAccessMask)
        .setOldLayout(layout)
        .setNewLayout(newLayout)
        .setImage(image)
        .setSubresourceRange(range);
    cmdbuf.pipelineBarrier(sourceStage, destinationStage, {}, {}, {}, barrier);

    layout = newLayout;
}

void Texture::createImage(uint32_t w, uint32_t h) {
    vk::ImageCreateInfo createInfo;
    createInfo.setImageType(vk::ImageType::e2D)
        .setArrayLayers(1)
        .setMipLevels(1)
        .setExtent({ w, h, 1 })
        .setFormat(format)
        .setTiling(vk::ImageTiling::eOptimal)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled)
        .setSamples(vk::SampleCountFlagBits::e1);
    image = Context::GetInstance().device.createImage(createInfo);
}

void Texture::allocMemory() {
    auto& device = Context::GetInstance().device;
    vk::MemoryAllocateInfo allocInfo;

    auto requirements = device.getImageMemoryRequirements(image);
    allocInfo.setAllocationSize(requirements.size);

    uint32_t index = 0;
    auto properties = Context::GetInstance().physicaldevice.getMemoryProperties();
    for (size_t i = 0; i < properties.memoryTypeCount; i++)
    {
        if ((1 << i) & requirements.memoryTypeBits && properties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)
        {
            index = i;
            break;
        }
    }
    allocInfo.setMemoryTypeIndex(index);

    memory = device.allocateMemory(allocInfo);
}


void Texture::transformData2Image(vk::CommandBuffer cmdbuf, Buffer& buffer, uint32_t w, uint32_t h) {
    vk::BufferImageCopy region;
    vk::ImageSubresourceLayers subresource;
    subresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
        .setBaseArrayLayer(0)
        .setLayerCount(1)
        .setMipLevel(0);
    region.setImageExtent(vk::Extent3D{ w, h, 1 })
        .setBufferImageHeight(0)
        .setBufferOffset(0)
        .setImageOffset(0)
        .setBufferRowLength(0)
        .setImageSubresource(subresource);

    cmdbuf.copyBufferToImage(buffer.buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
}

void Texture::transitionImageLayoutFromUndefine2Dst(vk::CommandBuffer buffer) 
{
    vk::ImageSubresourceRange range;
    range.setAspectMask(vk::ImageAspectFlagBits::eColor)
        .setBaseArrayLayer(0)
        .setBaseMipLevel(0)
        .setLayerCount(1)
        .setLevelCount(1);
    vk::ImageMemoryBarrier barrier;
    barrier.setImage(image)
        .setOldLayout(vk::ImageLayout::eUndefined)
        .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
        .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
        .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
        .setSubresourceRange(range)
        .setSrcAccessMask(vk::AccessFlagBits::eNone)
        .setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
    buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);
}

void Texture::transitionImageLayoutFromDst2Optimal(vk::CommandBuffer buffer) {
    vk::ImageSubresourceRange range;
    range.setAspectMask(vk::ImageAspectFlagBits::eColor)
        .setBaseArrayLayer(0)
        .setBaseMipLevel(0)
        .setLayerCount(1)
        .setLevelCount(1);
    vk::ImageMemoryBarrier barrier;
    barrier.setImage(image)
        .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
        .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
        .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
        .setSubresourceRange(range)
        .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
        .setDstAccessMask(vk::AccessFlagBits::eShaderRead);
    buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);
}

void Texture::transitionImageLayoutFromUndefine2Opt(vk::CommandBuffer buffer)
{
    vk::ImageSubresourceRange range;
    range.setAspectMask(vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil)
        .setBaseArrayLayer(0)
        .setBaseMipLevel(0)
        .setLayerCount(1)
        .setLevelCount(1);
    vk::ImageMemoryBarrier barrier;
    barrier.setImage(image)
        .setOldLayout(vk::ImageLayout::eUndefined)
        .setNewLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
        .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
        .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
        .setSubresourceRange(range)
        .setSrcAccessMask(vk::AccessFlagBits::eNone)
        .setDstAccessMask(vk::AccessFlagBits::eShaderWrite);
    buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);
}

void Texture::createImageView() {
    vk::ImageViewCreateInfo createInfo;
    vk::ComponentMapping mapping;
    vk::ImageSubresourceRange range;
    range.setAspectMask(vk::ImageAspectFlagBits::eColor)
        .setBaseArrayLayer(0)
        .setLayerCount(1)
        .setLevelCount(1)
        .setBaseMipLevel(0);
    createInfo.setImage(image)
        .setViewType(vk::ImageViewType::e2D)
        .setComponents(mapping)
        .setFormat(format)
        .setSubresourceRange(range);
    view = Context::GetInstance().device.createImageView(createInfo);
}

std::unique_ptr<TextureManager> TextureManager::instance_ = nullptr;

std::shared_ptr<Texture> TextureManager::Load(const std::string& filename) {
    datas_.push_back(std::shared_ptr<Texture>(new Texture(filename)));
    return datas_.back();
}

std::shared_ptr<Texture> TextureManager::LoadHDRCubemap(const std::string& filename, vk::Format format)
{
    datas_.push_back(std::shared_ptr<Texture>(new Texture(filename, format)));
    return datas_.back();
}

std::shared_ptr<Texture> TextureManager::Create(void* data, uint32_t w, uint32_t h, vk::Format format) {
    datas_.push_back(std::shared_ptr<Texture>(new Texture(data, w, h, format)));
    return datas_.back();
}

std::shared_ptr<Texture> TextureManager::Create(void* data, uint32_t w, uint32_t h, uint32_t channel, vk::Format format) {
    datas_.push_back(std::shared_ptr<Texture>(new Texture(data, w, h, channel, format)));
    return datas_.back();
}

std::shared_ptr<Texture> TextureManager::Create(uint32_t w, uint32_t h, vk::Format format, vk::ImageUsageFlags usage, int miplevels)
{
    datas_.push_back(std::shared_ptr<Texture>(new Texture(w, h, format, usage, miplevels)));
    return datas_.back();
}

void TextureManager::Clear() {
    for (auto t : datas_)
    {
        t.reset();
    }
    datas_.clear();
}

void TextureManager::Destroy(std::shared_ptr<Texture> texture) {
    auto it = std::find_if(datas_.begin(), datas_.end(),
        [&](const std::shared_ptr<Texture>& t) {
            return t.get() == texture.get();
        });
    if (it != datas_.end()) {
        Context::GetInstance().device.waitIdle();
        datas_.erase(it);
        texture.reset();
        return;
    }
}