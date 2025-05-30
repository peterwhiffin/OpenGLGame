#pragma once
#include "forward.h"
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

void updateEditorCamera(EditorState* editor, Scene* scene, RenderState* renderer);
void updateCamera(Scene* scene);