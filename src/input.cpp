#include <glfw/glfw3.h>
#include "input.h"

void updateInput(GLFWwindow* window, InputActions* actions) {
    actions->menu = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
}