#pragma once

#include <string>

class SystemManger
{
public:
	static void Init(uint32_t width, uint32_t height, std::string name = "demo");
	static void Shutdown();
};