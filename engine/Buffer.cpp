#include "Buffer.h"

#include "Context.h"

Buffer::Buffer(size_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags property)
	:size(size)
{
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
	for (int i = 0; i < properties.memoryTypeCount; i++)
	{
		if ((1 << i) & requirements.memoryTypeBits && properties.memoryTypes[i].propertyFlags & property)
		{
			info.index = i;
			break;
		}
	}
	return info;
}
