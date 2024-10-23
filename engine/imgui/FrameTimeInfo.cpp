#include "FrameTimeInfo.h"
#include "FrameTimer.h"
#include "imgui.h"

ImGuiFrameTimeInfo::ImGuiFrameTimeInfo(FrameTimer* pFrameTimer)
    :mFrameTimer(pFrameTimer)
{
}

void ImGuiFrameTimeInfo::viewMainMenu()
{
    if (ImGui::MenuItem("Frame Time Info"))
    {
        mOpen = !mOpen;
    }
}

void ImGuiFrameTimeInfo::customUI()
{
    if (!mOpen)
    {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiWindowFlags_NoScrollbar);
    ImGui::Begin("Frame Time Info", &mOpen);

    ImGui::Text("%.2f ms/frame (%.1f fps)",
        (double)mFrameTimer->averageFrameTime<std::chrono::nanoseconds>().count() /
        1000000., mFrameTimer->framePerSecond());

    ImGui::End();
}