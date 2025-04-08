#include <glfw/glfw3.h>
#include "input.h"

void updateInput(GLFWwindow* window, InputActions* actions) {
    actions->movement.x = 0;
    actions->movement.y = 0;

    actions->menu = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    actions->jump = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;

    actions->movement.x += glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ? -1 : 0;
    actions->movement.x += glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS ? 1 : 0;
    actions->movement.y += glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ? -1 : 0;
    actions->movement.y += glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ? 1 : 0;
}