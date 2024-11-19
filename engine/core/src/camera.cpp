#include "camera.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

std::shared_ptr<Camera> CameraManager::mainCamera = nullptr;

void CameraManager::init(glm::vec3 position)
{
	mainCamera.reset(new Camera(position));
}

void CameraUI::customUI()
{
	if (ImGui::CollapsingHeader("Main Camera Controller", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat3("Camera Position", glm::value_ptr(CameraManager::mainCamera->Position), -10.0f, 1000.0f);
	}
}
