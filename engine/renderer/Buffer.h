#pragma once

#include "vulkan/vulkan.hpp"

class Buffer {
public:
	vk::Buffer buffer;
	vk::DeviceMemory memory;
	size_t size;

	Buffer(size_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags property);
	~Buffer();

private:

	struct MemoryInfo
	{
		size_t size;
		uint32_t index;
	};
	bool use_devAddr  = false;
	void createBuffer(size_t size, vk::BufferUsageFlags usage);
	void allocateMemory(MemoryInfo info);
	void bindMemory2Buf();
	MemoryInfo queryMemoryInfo(vk::MemoryPropertyFlags property);
	
};
void CopyBuffer(vk::Buffer& src, vk::Buffer& dst, size_t size, size_t srcoffset, size_t dstoffset);
bool UploadBufferData(vk::Fence fence, std::shared_ptr<Buffer> buffer, size_t size, const void* data);
bool UploadBufferDataRange(vk::Fence fence, std::shared_ptr<Buffer> buffer, size_t offset, size_t size, const void* data);