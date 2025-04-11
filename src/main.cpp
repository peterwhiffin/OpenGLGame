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
void updateCamera(GLFWwindow* window, InputActions* input, CameraController* camera);
void setViewProjection(Camera* camera);
void onScreenChanged(GLFWwindow* window, int width, int height);
void initializeIMGUI(GLFWwindow* window);
void createEntityFromModel(Model* model, std::vector<Entity*>* entities, std::vector<MeshRenderer*>* renderers, Entity* newEntity, glm::vec3 scale, glm::vec3 position);
void createImGuiEntityTree(Entity* entity, ImGuiTreeNodeFlags node_flags, Entity** node_clicked);
void exitProgram(int code);

struct DirectionalLight {
    glm::vec3 position;
    glm::vec3 lookDirection;
    glm::vec3 color;
    glm::vec3 brightness;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};
int screenWidth = 800;
int screenHeight = 600;
float currentFrame = 0.0f;
float lastFrame = 0.0f;
float deltaTime = 0.0f;
Camera* mainCamera;
int counter = 0;
int main() {
    GLFWwindow* window = createContext();
    InputActions input = InputActions();
    initializeIMGUI(window);

    unsigned int defaultTexture;
    glGenTextures(1, &defaultTexture);
    unsigned char whitePixel[3] = {255, 255, 255};
    glBindTexture(GL_TEXTURE_2D, defaultTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, whitePixel);

    unsigned int defaultShader = loadShader("../src/shaders/litshader.vs", "../src/shaders/litshader.fs");
    glUniform1i(uniform_location::kTextureDiffuse, uniform_location::kTextureDiffuseUnit);
    glUniform1i(uniform_location::kTextureSpecular, uniform_location::kTextureSpecularUnit);
    glUniform1i(uniform_location::kTextureShadowMap, uniform_location::kTextureShadowMapUnit);
    glUniform1i(uniform_location::kTextureNoise, uniform_location::kTextureNoiseUnit);

    glUseProgram(defaultShader);
    DirectionalLight sun;
    sun.position = glm::vec3(-3.0f, 30.0f, -2.0f);
    sun.lookDirection = glm::vec3(0.0f, 0.0f, 0.0f);
    sun.color = glm::vec3(1.0f, 1.0f, 1.0f);
    sun.ambient = glm::vec3(0.21f);
    sun.diffuse = glm::vec3(0.94f);
    sun.specular = glm::vec3(0.65f);

    glUniform3fv(glGetUniformLocation(defaultShader, "dirLight.position"), 1, glm::value_ptr(sun.position));
    glUniform3fv(glGetUniformLocation(defaultShader, "dirLight.ambient"), 1, glm::value_ptr(sun.ambient));
    glUniform3fv(glGetUniformLocation(defaultShader, "dirLight.diffuse"), 1, glm::value_ptr(sun.diffuse));
    glUniform3fv(glGetUniformLocation(defaultShader, "dirLight.specular"), 1, glm::value_ptr(sun.specular));

    std::vector<MeshRenderer*> renderers;
    std::vector<Entity*> entities;
    std::vector<Texture> allTextures;

    Texture default;
    default.id = defaultTexture;
    default.path = "default";
    allTextures.push_back(default);

    Model* sponzaModel = loadModel("../resources/models/sponza/sponza.obj", &allTextures);
    Model* m4Model = loadModel("../resources/models/M4/ddm4 v7.obj", &allTextures);

    createEntityFromModel(sponzaModel, &entities, &renderers, nullptr, glm::vec3(0.01f, 0.01f, 0.01f), glm::vec3(0.0f));
    createEntityFromModel(m4Model, &entities, &renderers, nullptr, glm::vec3(0.1f), glm::vec3(0.0f, 15.0f, 0.0f));
    /*
        for (int i = 0; i < sponzaModel->meshes.size(); i++) {
            Entity* newEntity = new Entity();
            MeshRenderer* meshRenderer = new MeshRenderer(newEntity, &sponzaModel->meshes[i], &sponzaModel->meshes[i].material);
            newEntity->transform.scale = glm::vec3(0.01f, 0.01f, 0.01f);

            entities.push_back(newEntity);
            renderers.push_back(meshRenderer);
        }
     */
    std::cout << "entity count: " << counter << std::endl;
    Entity* playerEntity = new Entity();
    Camera camera(playerEntity, glm::radians(68.0f), (float)screenWidth / screenHeight, 0.1, 10000);
    mainCamera = &camera;
    CameraController cameraController(playerEntity, camera);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    bool enableDemoWindow = false;
    bool enableDirLight = false;
    float dirLightBrightness = 1.0f;
    float ambientBrightness = 0.21f;
    Entity* nodeClicked = nullptr;

    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow |
                                   ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                   ImGuiTreeNodeFlags_SpanAvailWidth |
                                   ImGuiTreeNodeFlags_DefaultOpen;
    while (!glfwWindowShouldClose(window)) {
        currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        updateInput(window, &input);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("The Window");
        ImGui::Text("This is a ImGui window");
        ImGui::Checkbox("Enable Demo Window", &enableDemoWindow);
        ImGui::Checkbox("Enable Directional Light", &enableDirLight);
        ImGui::SliderFloat("Directional Light Brightness", &dirLightBrightness, 0.0f, 10.0f);
        ImGui::SliderFloat("Ambient Brightness", &ambientBrightness, 0.0f, 3.0f);
        ImGui::SliderFloat("Move Speed", &cameraController.moveSpeed, 0.0f, 25.0f);

        // createImGuiEntityTree(entities[0], nodeFlags, &nodeClicked);

        for (Entity* entity : entities) {
            createImGuiEntityTree(entity, nodeFlags, &nodeClicked);
        }

        if (enableDemoWindow) {
            ImGui::ShowDemoWindow();
        }
        ImGui::End();

        sun.ambient = glm::vec3(ambientBrightness);

        updateCamera(window, &input, &cameraController);
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
            glUniform4fv(uniform_location::kBaseColor, 1, glm::value_ptr(renderer->material->baseColor));
            glUniform1f(uniform_location::kShininess, renderer->material->shininess);
            glUniform3fv(uniform_location::kViewPos, 1, glm::value_ptr(camera.transform->position));

            glUniform1i(glGetUniformLocation(defaultShader, "dirLight.enabled"), enableDirLight);
            glUniform3fv(glGetUniformLocation(defaultShader, "dirLight.ambient"), 1, glm::value_ptr(sun.ambient));
            glUniform3fv(glGetUniformLocation(defaultShader, "dirLight.diffuse"), 1, glm::value_ptr(sun.diffuse * dirLightBrightness));
            glUniform3fv(glGetUniformLocation(defaultShader, "dirLight.specular"), 1, glm::value_ptr(sun.specular * dirLightBrightness));

            glActiveTexture(uniform_location::kTextureDiffuseUnit);
            glBindTexture(GL_TEXTURE_2D, renderer->material->textures[0].id);
            glBindVertexArray(renderer->mesh->VAO);
            glDrawElements(GL_TRIANGLES, renderer->mesh->indices.size(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    exitProgram(0);
    return 0;
}

void createImGuiEntityTree(Entity* entity, ImGuiTreeNodeFlags node_flags, Entity** node_clicked) {
    ImGui::PushID(entity);
    bool node_open = ImGui::TreeNodeEx(entity->name.c_str(), node_flags);

    if (ImGui::IsItemClicked()) {
        *node_clicked = entity;
    }

    if (node_open) {
        for (Entity* child : entity->children) {
            createImGuiEntityTree(child, node_flags, node_clicked);
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void createEntityFromModel(Model* model, std::vector<Entity*>* entities, std::vector<MeshRenderer*>* renderers, Entity* parentEntity, glm::vec3 scale, glm::vec3 position) {
    if (model->parent == nullptr) {
        parentEntity = new Entity();
        parentEntity->name = model->name;
        entities->push_back(parentEntity);
    }

    for (int i = 0; i < model->children.size(); i++) {
        Entity* childEntity = new Entity();
        childEntity->name = "poo poo head";
        if (model->children[i]->hasMesh) {
            MeshRenderer* meshRenderer = new MeshRenderer(childEntity, &model->children[i]->mesh, &model->children[i]->mesh.material);
            renderers->push_back(meshRenderer);
            childEntity->name = model->children[i]->mesh.name;
        }

        childEntity->parent = parentEntity;
        childEntity->transform.scale = scale;
        childEntity->transform.position = position;

        parentEntity->children.push_back(childEntity);

        for (int j = 0; j < model->children[i]->children.size(); j++) {
            createEntityFromModel(model->children[i], entities, renderers, childEntity, scale, position);
        }
    }

    /*
        for (int i = 0; i < model->children.size(); i++) {
            Entity* childEntity = new Entity();
            childEntity->parent = newEntity;
            newEntity->children.push_back(childEntity);
            createEntityFromModel(model->children[i], entities, renderers, childEntity);
        } */
}

void updateCamera(GLFWwindow* window, InputActions* input, CameraController* camera) {
    if (!input->altFire) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        return;
    }

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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

    glfwSetFramebufferSizeCallback(window, onScreenChanged);
    return window;
}

void onScreenChanged(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    screenWidth = width;
    screenHeight = height;
    mainCamera->aspectRatio = (float)screenWidth / screenHeight;
}

void initializeIMGUI(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");
}

void exitProgram(int code) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    exit(code);
}