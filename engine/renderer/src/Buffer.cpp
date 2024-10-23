#include "Buffer.h"

#include "Context.h"
#include "CommandBuffer.h"

Buffer::Buffer(size_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags property)
	:size(size)
{
	if (usage & vk::BufferUsageFlagBits::eShaderDeviceAddress)
	{
		use_devAddr = true;
	}
	createBuffer(size, usage);
	auto info = queryMemoryInfo(property);
	allocateMemory(info);
	bindMemory2Buf();
}

Buffer::~Buffer()
{
	auto device = Context::GetInstance().device;
	device.freeMemory(memory);
	device.destroyBuffer(buffer);
}

void Buffer::createBuffer(size_t size, vk::BufferUsageFlags usage)
{
	vk::BufferCreateInfo bufferCI;
	bufferCI.setSize(size)
		.setUsage(usage)
		.setSharingMode(vk::SharingMode::eExclusive);
	buffer = Context::GetInstance().device.createBuffer(bufferCI);
}

void Buffer::allocateMemory(MemoryInfo info)
{
	vk::MemoryAllocateInfo memoryAI;
	memoryAI.setAllocationSize(info.size)
		.setMemoryTypeIndex(info.index);
	if (use_devAddr)
	{
		vk::MemoryAllocateFlagsInfo allocateFlags;
		allocateFlags.setFlags(vk::MemoryAllocateFlagBits::eDeviceAddress);
		memoryAI.setPNext(&allocateFlags);
	}
	memory = Context::GetInstance().device.allocateMemory(memoryAI);
}

void Buffer::bindMemory2Buf()
{
	Context::GetInstance().device.bindBufferMemory(buffer, memory, 0);
}

Buffer::MemoryInfo Buffer::queryMemoryInfo(vk::MemoryPropertyFlags property)
{
	MemoryInfo info{};
	auto requirements = Context::GetInstance().device.getBufferMemoryRequirements(buffer);
	info.size = requirements.size;

	auto properties = Context::GetInstance().physicaldevice.getMemoryProperties();
	for (uint32_t i = 0; i < properties.memoryTypeCount; ++i)
	{
		if ((1 << i) & requirements.memoryTypeBits && properties.memoryTypes[i].propertyFlags & property)
		{
			info.index = i;
			break;
		}
	}
	return info;
}

void CopyBuffer(vk::Buffer& src, vk::Buffer& dst, size_t size, size_t srcoffset, size_t dstoffset)
{
	auto cmdbuf = CommandManager::BeginSingle(Context::GetInstance().graphicsCmdPool);
	vk::BufferCopy region;
	region.setSize(size)
		.setSrcOffset(srcoffset)
		.setDstOffset(dstoffset);
	cmdbuf.copyBuffer(src, dst, region);
	CommandManager::EndSingle(Context::GetInstance().graphicsCmdPool, cmdbuf, Context::GetInstance().graphicsQueue);
}

bool UploadBufferData(vk::Fence fence, std::shared_ptr<Buffer> buffer, size_t size, const void* data)
{
	Buffer staging(size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
	void* p = Context::GetInstance().device.mapMemory(staging.memory, 0, size);
	memcpy(p, data, size);
	Context::GetInstance().device.unmapMemory(staging.memory);
	Context::GetInstance().graphicsQueue.waitIdle();
	CopyBuffer(staging.buffer, buffer->buffer, size, 0, 0);
	return true;
}
