#pragma once
#include "ImGuiBase.h"
class FrameTimer;
class ImGuiFrameTimeInfo :
    public ImGuiBase
{
public:
    ImGuiFrameTimeInfo(FrameTimer* pFrameTimer);

private:
    void viewMainMenu() final;
    void customUI() final;

    FrameTimer const* const mFrameTimer;

    bool mOpen{ true };
};

