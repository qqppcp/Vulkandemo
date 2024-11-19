#pragma once
#include "application.h"
#include "FrameTimer.h"

class ForwardShade : public Application
{
public:
	virtual void Init(uint32_t width, uint32_t height) override;
	virtual void Shutdown() override;
	virtual void run() override;
private:
	struct AppState
	{
		bool running;
		bool suspended;
		int width;
		int height;
		FrameTimer timer;
	} state;
};