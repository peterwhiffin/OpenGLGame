#pragma once
#include "utils/mathutils.h"

struct Scene;

struct InputActions {
    glm::vec2 movement = glm::vec2(0.0f, 0.0f);
    glm::dvec2 cursorPosition = glm::dvec2(0, 0);

    double lookX = 0;
    double lookY = 0;
    double oldX = 0;
    double oldY = 0;

    bool menu = true;
    bool spawn = false;
    bool jump = false;
    bool fire = false;
    bool altFire = false;
    bool deleteKey = false;
};

void updateInput(Scene* scene);