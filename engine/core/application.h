#pragma once
#include <cstdint>

class Application
{
public:
	virtual ~Application() = default;
	virtual void Init(uint32_t width, uint32_t height) = 0;
	virtual void Shutdown() = 0;
	virtual void run() = 0;
};