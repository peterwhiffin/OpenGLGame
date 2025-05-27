#pragma once
#include "utils/mathutils.h"
struct Scene;
struct RenderState;

struct Camera {
    uint32_t entityID;
    float fov;
    float fovRadians;
    float nearPlane;
    float farPlane;
};

void updateCamera(Scene* scene);