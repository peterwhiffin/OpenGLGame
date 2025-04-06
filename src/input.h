#ifndef INPUT_H
#define INPUT_H

#include <glfw/glfw3.h>

struct InputActions {
  bool menu;
};

void updateInput(GLFWwindow* window, InputActions* actions);

#endif