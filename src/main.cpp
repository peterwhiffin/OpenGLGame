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
float moveSpeed = 200.0f;
float currentFrame = 0.0f;
float lastFrame = 0.0f;
float deltaTime = 0.0f;

int main() {
    GLFWwindow* window = createContext();
    InputActions input = InputActions();

    unsigned int defaultShader = loadShader("../src/shaders/litshader.vs", "../src/shaders/litshader.fs");

    std::vector<Mesh*> renderers;
    std::vector<Entity*> entities;
    std::vector<Texture> allTextures;

    Model* sponzaModel = loadModel("../resources/models/sponza/sponza.obj", &allTextures);

    Entity sponza = Entity();
    sponza.transform.position = glm::vec3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < sponzaModel->meshes.size(); i++) {
        Entity newEntity;
        entities.push_back(&newEntity);
        newEntity.components.push_back(&sponzaModel->meshes[i]);
        sponzaModel->meshes[i].entity = &newEntity;
        sponzaModel->meshes[i].transform = &newEntity.transform;
        renderers.push_back(&sponzaModel->meshes[i]);
    }

    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, -1400.0f), glm::vec3(0.0f, 100.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(68.0f), (float)screenWidth / screenHeight, 0.1f, 10000.0f);
    glEnable(GL_DEPTH_TEST);
    while (!glfwWindowShouldClose(window)) {
        currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        updateInput(window, &input);

        if (input.menu) {
            glfwSetWindowShouldClose(window, true);
        }

        cameraPos += glm::vec3(input.movement.x * deltaTime * moveSpeed, input.jump * deltaTime * moveSpeed, input.movement.y * deltaTime * moveSpeed);

        glViewport(0, 0, screenWidth, screenHeight);
        glClearColor(0.34, 0.34, 0.8, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        view = glm::lookAt(cameraPos, glm::vec3(0, 0, 0), glm::vec3(0.0f, 1.0f, 0.0f));

        for (Mesh* renderer : renderers) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, renderer->transform->position);
            glm::mat4 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
            glUseProgram(defaultShader);

            glUniformMatrix4fv(uniform_location::kViewMatrix, 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(uniform_location::kProjectionMatrix, 1, GL_FALSE, glm::value_ptr(projection));
            glUniformMatrix4fv(uniform_location::kModelMatrix, 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix4fv(uniform_location::kNormalMatrix, 1, GL_FALSE, glm::value_ptr(normalMatrix));
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, renderer->material.textures[0].id);
            glBindVertexArray(renderer->VAO);
            glDrawElements(GL_TRIANGLES, renderer->indices.size(), GL_UNSIGNED_INT, 0);
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