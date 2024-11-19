#pragma once

#include <string>

class SystemManger
{
public:
	void Init(uint32_t width, uint32_t height, std::string name = "demo");
	void Shutdown();
};