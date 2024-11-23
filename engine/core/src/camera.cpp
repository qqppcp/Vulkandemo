#include "camera.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace {
	static glm::mat4 jitterMat_{ 1.0f };

	static float VanDerCorputGenerator(size_t base, size_t index) {
		float ret = 0.0f;
		float denominator = float(base);
		while (index > 0) {
			size_t multiplier = index % base;
			ret += float(multiplier) / denominator;
			index = index / base;
			denominator *= base;
		}
		return ret;
	}
}

std::shared_ptr<Camera> CameraManager::mainCamera = nullptr;

void CameraManager::init(glm::vec3 position)
{
	mainCamera.reset(new Camera(position));
}

glm::mat4 CameraManager::JitterMat(uint32_t frameIndex, int numSamples, int width, int height)
{
	uint32_t index = (frameIndex % numSamples) + 1;
	auto x = VanDerCorputGenerator(2, index) - 0.5f;
	auto y = VanDerCorputGenerator(3, index) - 0.5f;

	float uvOffsetX = float(x) / width;
	float uvOffsetY = float(y) / height;
	float ndcOffsetX = uvOffsetX * 2.0f;
	float ndcOffsetY = uvOffsetY * 2.0f;

	jitterMat_[2][0] = ndcOffsetX;
	jitterMat_[2][1] = ndcOffsetY;
	return jitterMat_;
}

void CameraUI::customUI()
{
	if (ImGui::CollapsingHeader("Main Camera Controller", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat3("Camera Position", glm::value_ptr(CameraManager::mainCamera->Position), -10.0f, 1000.0f);
	}
}
