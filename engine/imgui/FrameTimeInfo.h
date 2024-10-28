#pragma once
#include "ImGuiBase.h"
class FrameTimer;
class ImGuiFrameTimeInfo :
    public ImGuiBase
{
public:
    ImGuiFrameTimeInfo(FrameTimer* pFrameTimer);

private:
    virtual void viewMainMenu() final;
    virtual void customUI() final;

    FrameTimer const* const mFrameTimer;

    bool mOpen{ true };
};

