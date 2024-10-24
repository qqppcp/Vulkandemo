#include "camera.h"

std::shared_ptr<Camera> CameraManager::mainCamera = nullptr;

void CameraManager::init(glm::vec3 position)
{
	mainCamera.reset(new Camera(position));
}
