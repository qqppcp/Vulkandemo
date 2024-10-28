#include "termination.h"
#include "imgui.h"
bool ImGuiTermination::terminationRequested() const
{
    return mTerminate;
}

void ImGuiTermination::fileMainMenu()
{
    if (ImGui::MenuItem("Exit"))
    {
        mTerminate = true;
    }
}