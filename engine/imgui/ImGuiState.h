#pragma once
#include <mutex>
#include <vector>
class ImGuiBase;

class ImGuiState
{
public:
    bool addUI(ImGuiBase* pUI);

    void removeUI(ImGuiBase* pUI);

    void runUI();

private:
    std::mutex mSync;

    std::vector<ImGuiBase*> mUI;

};