#pragma once

#include "ImGuiBase.h"

class ImGuiTermination : public ImGuiBase
{
public:
    bool terminationRequested() const;

private:
    void fileMainMenu() final;

    bool mTerminate{ false };

};
