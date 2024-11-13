#pragma once
#include <glm/glm.hpp>
#include <glm/gtx/type_aligned.hpp>

#include "camera.h"

struct LightData {
    glm::aligned_vec4 lightPos = glm::vec4(-9.0, 2.0, 2.0, 1.0);
    glm::aligned_vec4 lightDir = glm::vec4(0.0, 1.0, 0.0, 1.0);
    glm::aligned_vec4 lightColor = {};
    glm::aligned_vec4 ambientColor = {};
    glm::aligned_mat4 lightVP;
    float innerAngle = 0.523599f;  // 30 degree
    float outerAngle = 1.22173f;   // 70 degree
    Camera lightCam;
};