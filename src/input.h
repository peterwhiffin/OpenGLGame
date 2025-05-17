#pragma once
// #include "component.h"
#include <glm/vec2.hpp>
#include <glm/common.hpp>
#include <glm/glm.hpp>
#include <glfw/glfw3.h>

struct InputActions {
    bool menu = true;
    bool spawn = false;
    bool jump = false;
    bool fire = false;
    bool altFire = false;
    bool deleteKey = false;
    glm::vec2 movement = glm::vec2(0.0f, 0.0f);
    double lookX = 0;
    double lookY = 0;
    glm::dvec2 cursorPosition = glm::dvec2(0, 0);
};

void updateInput(GLFWwindow* window, InputActions* actions);