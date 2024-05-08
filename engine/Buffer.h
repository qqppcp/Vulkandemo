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

	void createBuffer(size_t size, vk::BufferUsageFlags usage);
	void allocateMemory(MemoryInfo info);
	void bindMemory2Buf();
	MemoryInfo queryMemoryInfo(vk::MemoryPropertyFlags property);
	
};