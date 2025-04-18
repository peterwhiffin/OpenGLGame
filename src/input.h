#ifndef INPUT_H
#define INPUT_H

#include <glfw/glfw3.h>
#include <glm/glm.hpp>

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

#endif