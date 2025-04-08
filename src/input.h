#ifndef INPUT_H
#define INPUT_H

#include <glfw/glfw3.h>
#include <glm/glm.hpp>

struct InputActions {
    bool menu;
    bool jump;
    glm::vec2 movement;
    glm::vec2 look;
};

void updateInput(GLFWwindow* window, InputActions* actions);

#endif