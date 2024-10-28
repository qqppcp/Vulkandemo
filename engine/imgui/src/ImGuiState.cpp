#include "ImGuiState.h"
#include "ImGuiBase.h"
#include "imgui.h"

bool ImGuiState::addUI(ImGuiBase* pUI)
{
    if (pUI == nullptr)
    {
        return false;
    }

    std::scoped_lock lock{ mSync };

    // Check to see if its already in
    for (auto const* it : mUI)
    {
        if (it == pUI)
        {
            return false;
        }
    }

    mUI.emplace_back(pUI);

    return true;
}

void ImGuiState::removeUI(ImGuiBase* pUI)
{
    std::scoped_lock lock{ mSync };

    for (auto it = mUI.begin(); it != mUI.end(); it++)
    {
        if (*it == pUI)
        {
            mUI.erase(it);
            return;
        }
    }
}

void ImGuiState::runUI()
{
    std::scoped_lock lock{ mSync };

    ImGui::BeginMainMenuBar();

    // File menu
    if (ImGui::BeginMenu("File"))
    {
        for (auto* it : mUI)
        {
            it->fileMainMenu();
        }

        ImGui::EndMenu();
    }

    // View menu
    if (ImGui::BeginMenu("View"))
    {
        for (auto* it : mUI)
        {
            it->viewMainMenu();
        }

        ImGui::EndMenu();
    }

    // Custom menus
    for (auto* it : mUI)
    {
        it->customMainMenu();
    }

    ImGui::EndMainMenuBar();

    // Custom UI
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar);
    ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize);
    for (auto* it : mUI)
    {
        it->customUI();
    }
    ImGui::End();
    ImGui::PopStyleVar();
}