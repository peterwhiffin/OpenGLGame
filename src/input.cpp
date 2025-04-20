#include "component.h"
#include "input.h"

double oldX = 0;
double oldY = 0;

void updateInput(GLFWwindow* window, InputActions* actions) {
    actions->movement.x = 0;
    actions->movement.y = 0;

    actions->menu = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    actions->jump = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    actions->spawn = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;

    actions->movement.x += glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ? -1 : 0;
    actions->movement.x += glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS ? 1 : 0;
    actions->movement.y += glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ? -1 : 0;
    actions->movement.y += glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ? 1 : 0;

    actions->fire = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS ? true : false;
    actions->altFire = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS ? true : false;

    if (actions->movement != glm::vec2(0.0f)) {
        actions->movement = glm::normalize(actions->movement);
    }

    glfwGetCursorPos(window, &actions->cursorPosition.x, &actions->cursorPosition.y);

    actions->lookX = actions->cursorPosition.x - oldX;
    actions->lookY = actions->cursorPosition.y - oldY;

    oldX = actions->cursorPosition.x;
    oldY = actions->cursorPosition.y;
}