#pragma once
#include "component.h"

struct InputActions {
    bool menu;
    bool spawn;
    bool jump;
    bool fire;
    bool altFire;
    glm::vec2 movement;
    double lookX;
    double lookY;
    glm::dvec2 cursorPosition;
};

void updateInput(GLFWwindow* window, InputActions* actions);