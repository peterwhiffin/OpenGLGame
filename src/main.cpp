#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <glm/ext/quaternion_float.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include "utils/imgui.h"
#include "utils/imgui_impl_glfw.h"
#include "utils/imgui_impl_opengl3.h"
#include "input.h"
#include "loader.h"
#include "component.h"
#include "shader.h"

GLFWwindow* createContext();
void updateCamera(InputActions* input, CameraController* camera);
void setViewProjection(Camera* camera);
void exitProgram(int code);

int screenWidth = 800;
int screenHeight = 600;
float currentFrame = 0.0f;
float lastFrame = 0.0f;
float deltaTime = 0.0f;

int main() {
    GLFWwindow* window = createContext();
    InputActions input = InputActions();

    unsigned int defaultSpecTex;
    glGenTextures(1, &defaultSpecTex);
    unsigned char blackPixel[3] = {0, 0, 0};
    glBindTexture(GL_TEXTURE_2D, defaultSpecTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, blackPixel);

    unsigned int defaultShader = loadShader("../src/shaders/litshader.vs", "../src/shaders/litshader.fs");

    std::vector<MeshRenderer*> renderers;
    std::vector<Entity*> entities;
    std::vector<Texture> allTextures;

    Texture defaultTexture;
    defaultTexture.id = defaultSpecTex;
    allTextures.push_back(defaultTexture);

    Model* sponzaModel = loadModel("../resources/models/sponza/sponza.obj", &allTextures);

    for (int i = 0; i < sponzaModel->meshes.size(); i++) {
        Entity* newEntity = new Entity();
        MeshRenderer* meshRenderer = new MeshRenderer(newEntity, &sponzaModel->meshes[i], &sponzaModel->meshes[i].material);
        newEntity->transform.scale = glm::vec3(0.01f, 0.01f, 0.01f);
        // newEntity->components.push_back(meshRenderer);

        entities.push_back(newEntity);
        renderers.push_back(meshRenderer);
    }

    Entity* playerEntity = new Entity();
    Camera camera(playerEntity, glm::radians(68.0f), (float)screenWidth / screenHeight, 0.1, 10000);
    CameraController cameraController(playerEntity, camera);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    while (!glfwWindowShouldClose(window)) {
        currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        updateInput(window, &input);

        if (input.menu) {
            glfwSetWindowShouldClose(window, true);
        }

        updateCamera(&input, &cameraController);
        setViewProjection(&camera);

        glViewport(0, 0, screenWidth, screenHeight);
        glClearColor(0.34, 0.34, 0.8, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (MeshRenderer* renderer : renderers) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), renderer->transform->position);
            model *= glm::mat4_cast(renderer->transform->rotation);
            model = glm::scale(model, renderer->transform->scale);
            glm::mat4 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));

            glUseProgram(defaultShader);
            glUniformMatrix4fv(uniform_location::kModelMatrix, 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix4fv(uniform_location::kViewMatrix, 1, GL_FALSE, glm::value_ptr(camera.viewMatrix));
            glUniformMatrix4fv(uniform_location::kProjectionMatrix, 1, GL_FALSE, glm::value_ptr(camera.projectionMatrix));
            glUniformMatrix4fv(uniform_location::kNormalMatrix, 1, GL_FALSE, glm::value_ptr(normalMatrix));
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, renderer->material->textures[0].id);

            glBindVertexArray(renderer->mesh->VAO);
            glDrawElements(GL_TRIANGLES, renderer->mesh->indices.size(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}

void updateCamera(InputActions* input, CameraController* camera) {
    float xOffset = input->lookX * camera->sensitivity;
    float yOffset = input->lookY * camera->sensitivity;
    float pitch = camera->pitch;
    float yaw = camera->yaw;

    yaw -= xOffset;
    pitch -= yOffset;

    if (pitch > 89.0f) {
        pitch = 89.0f;
    }

    if (pitch < -89.0f) {
        pitch = -89.0f;
    }

    camera->pitch = pitch;
    camera->yaw = yaw;

    float upTest = 0.0f;
    if (input->jump) {
        upTest = 1.0f;
    }

    glm::vec3 euler = glm::vec3(glm::radians(camera->pitch), glm::radians(camera->yaw), 0.0f);
    camera->transform->rotation = glm::quat(euler);

    glm::vec3 moveDir = glm::vec3(0.0f);
    moveDir += input->movement.y * forward(camera->transform) + input->movement.x * right(camera->transform);

    camera->transform->position += moveDir * camera->moveSpeed * deltaTime;
}

void setViewProjection(Camera* camera) {
    glm::vec3 position = camera->transform->position;
    camera->viewMatrix = glm::lookAt(position, position + forward(camera->transform), up(camera->transform));
    camera->projectionMatrix = glm::perspective(camera->fov, camera->aspectRatio, camera->nearPlane, camera->farPlane);
}

GLFWwindow* createContext() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwSwapInterval(0);
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Pete's Game", NULL, NULL);

    if (window == NULL) {
        std::cout << "Failed to create window" << std::endl;
        exitProgram(-1);
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to load GLAD" << std::endl;
        exitProgram(-1);
    }

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    return window;
}

void exitProgram(int code) {
    glfwTerminate();
    exit(code);
}