#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <vector>
#include <glm/gtc/type_ptr.hpp>

#include "input.h"
#include "loader.h"
#include "component.h"
#include "shader.h"

GLFWwindow* createContext();

int screenWidth = 800;
int screenHeight = 600;
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, -700.0f);

int main() {
    GLFWwindow* window = createContext();
    InputActions input = InputActions();

    unsigned int defaultShader = loadShader("../src/shaders/litshader.vs", "../src/shaders/litshader.fs");

    std::vector<MeshRenderer> renderers;
    std::vector<Texture> allTextures;

    Model* sponza = loadModel("../resources/models/sponza/sponza.obj", &allTextures);

    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, -1400.0f), glm::vec3(0.0f, 100.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(68.0f), (float)screenWidth / screenHeight, 0.1f, 1000.0f);

    while (!glfwWindowShouldClose(window)) {
        updateInput(window, &input);

        if (input.menu) {
            glfwSetWindowShouldClose(window, true);
        }

        glViewport(0, 0, screenWidth, screenHeight);
        glClearColor(0.34, 0.34, 0.8, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));

        for (Mesh mesh : sponza->meshes) {
            glUseProgram(defaultShader);
            glUniformMatrix4fv(uniform_location::kViewMatrix, 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(uniform_location::kProjectionMatrix, 1, GL_FALSE, glm::value_ptr(projection));
            glUniformMatrix4fv(uniform_location::kModelMatrix, 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix4fv(uniform_location::kNormalMatrix, 1, GL_FALSE, glm::value_ptr(normalMatrix));
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, sponza->materials[3].textures[0].id);
            glBindVertexArray(mesh.VAO);
            glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

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