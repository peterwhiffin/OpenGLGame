#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <vector>

#include "input.h"
#include "loader.h"
#include "component.h"

GLFWwindow* createContext();

int screenWidth = 800;
int screenHeight = 600;

int main() {
    GLFWwindow* window = createContext();
    InputActions input = InputActions();
    std::vector<MeshRenderer> renderers;

    while (!glfwWindowShouldClose(window)) {
        updateInput(window, &input);

        if (input.menu) {
            glfwSetWindowShouldClose(window, true);
        }

        glViewport(0, 0, screenWidth, screenHeight);
        glClearColor(0.34, 0.34, 0.8, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}

GLFWwindow* createContext() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Pete's Game", NULL, NULL);

    if (window == NULL) {
        std::cout << "Failed to create window" << std::endl;
        glfwTerminate();
        exit(-1);
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to load GLAD" << std::endl;
        glfwTerminate();
        exit(-1);
    }

    return window;
}