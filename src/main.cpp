#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext/quaternion_float.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <typeinfo>
#include <unordered_map>
#include "utils/imgui.h"
#include "utils/imgui_impl_glfw.h"
#include "utils/imgui_impl_opengl3.h"
#include "input.h"
#include "loader.h"
#include "component.h"
#include "camera.h"
#include "physics.h"
#include "animation.h"
#include "player.h"
#include "renderer.h"
#include "transform.h"
#include "shader.h"

GLFWwindow* createContext();
void onScreenChanged(GLFWwindow* window, int width, int height);
void initializeIMGUI(GLFWwindow* window);
void createImGuiEntityTree(Entity* entity, ImGuiTreeNodeFlags node_flags, Entity** node_clicked);
void exitProgram(int code);
bool searchEntities(Entity* entity, unsigned int id);

Entity* levelEntity;
Entity* trashCanEntity;
Entity* wrenchEntity;
Entity* nodeClicked = nullptr;
Player* player;
DirectionalLight sun;

glm::dvec2 pickPosition = glm::dvec2(0, 0);
glm::vec3 wrenchOffset = glm::vec3(0.3f, -0.3f, -0.5f);

float currentFrame = 0.0f;
float lastFrame = 0.0f;
float deltaTime = 0.0f;
float gravity = -18.81f;
float dirLightBrightness = 2.1f;
float ambientBrightness = 1.21f;

unsigned int nextEntityID = 1;
unsigned int pickingShader;

int screenWidth = 800;
int screenHeight = 600;

bool enableDirLight = true;
bool isPicking = false;
bool canSpawn = true;

int main() {
    GLFWwindow* window = createContext();
    InputActions input = InputActions();
    initializeIMGUI(window);

    unsigned int whiteTexture;
    unsigned int blackTexture;
    glGenTextures(1, &whiteTexture);
    glGenTextures(1, &blackTexture);
    unsigned char whitePixel[4] = {255, 255, 255, 255};
    unsigned char blackPixel[4] = {0, 0, 0, 255};
    glBindTexture(GL_TEXTURE_2D, whiteTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
    glBindTexture(GL_TEXTURE_2D, blackTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, blackPixel);

    unsigned int defaultShader = loadShader("../src/shaders/litshader.vs", "../src/shaders/litshader.fs");
    pickingShader = loadShader("../src/shaders/pickingshader.vs", "../src/shaders/pickingshader.fs");
    // unsigned int pickingShader = loadShader("../src/shaders/litshader.vs", "../src/shaders/litshader.fs");
    glUseProgram(defaultShader);
    glUniform1i(uniform_location::kTextureDiffuse, uniform_location::kTextureDiffuseUnit);
    glUniform1i(uniform_location::kTextureSpecular, uniform_location::kTextureSpecularUnit);
    glUniform1i(uniform_location::kTextureShadowMap, uniform_location::kTextureShadowMapUnit);
    glUniform1i(uniform_location::kTextureNoise, uniform_location::kTextureNoiseUnit);

    sun.position = glm::vec3(-3.0f, 30.0f, -2.0f);
    sun.lookDirection = glm::vec3(0.0f, 0.0f, 0.0f);
    sun.color = glm::vec3(1.0f, 1.0f, 1.0f);
    sun.ambient = glm::vec3(0.21f);
    sun.diffuse = glm::vec3(0.94f);
    sun.specular = glm::vec3(0.18f);

    glUniform3fv(glGetUniformLocation(defaultShader, "dirLight.position"), 1, glm::value_ptr(sun.position));
    glUniform3fv(glGetUniformLocation(defaultShader, "dirLight.ambient"), 1, glm::value_ptr(sun.ambient));
    glUniform3fv(glGetUniformLocation(defaultShader, "dirLight.diffuse"), 1, glm::value_ptr(sun.diffuse));
    glUniform3fv(glGetUniformLocation(defaultShader, "dirLight.specular"), 1, glm::value_ptr(sun.specular));

    std::vector<MeshRenderer*> renderers;
    std::vector<Entity*> entities;
    std::vector<Texture> allTextures;
    std::vector<BoxCollider*> allColliders;
    std::vector<RigidBody*> rigidbodies;
    std::vector<Animator*> animators;
    std::vector<Camera*> cameras;

    Texture white;
    Texture black;
    white.id = whiteTexture;
    black.id = blackTexture;
    white.path = "white";
    black.path = "black";
    allTextures.push_back(white);
    allTextures.push_back(black);

    Model* testRoom = loadModel("../resources/models/testroom/testroom.gltf", &allTextures, defaultShader);
    Model* wrench = loadModel("../resources/models/wrench/wrench.gltf", &allTextures, defaultShader);
    Model* trashcan = loadModel("../resources/models/trashcan/trashcan.gltf", &allTextures, defaultShader);

    trashCanEntity = createEntityFromModel(trashcan, trashcan->rootNode, &renderers, nullptr, &allColliders, true, true, &nextEntityID);
    levelEntity = createEntityFromModel(testRoom, testRoom->rootNode, &renderers, nullptr, &allColliders, true, true, &nextEntityID);
    wrenchEntity = createEntityFromModel(wrench, wrench->rootNode, &renderers, nullptr, &allColliders, true, false, &nextEntityID);

    entities.push_back(levelEntity);
    entities.push_back(wrenchEntity);
    entities.push_back(trashCanEntity);

    addAnimator(wrenchEntity, wrench, &animators);
    addRigidBody(trashCanEntity, 10.0f, 5.0f, 25.0f, &rigidbodies);

    setPosition(&trashCanEntity->transform, glm::vec3(1.0f, 3.0f, 2.0f));
    BoxCollider* collider = (BoxCollider*)trashCanEntity->transform.children[0]->entity->components[component::kBoxCollider];
    collider->isActive = false;

    player = createPlayer(&nextEntityID, (float)screenWidth / screenHeight, &entities, &allColliders, &rigidbodies, &cameras);
    setParent(&wrenchEntity->transform, player->cameraController->cameraTarget);
    setLocalRotation(&wrenchEntity->transform, glm::quat(glm::vec3(glm::radians(0.0f), glm::radians(180.0f), 0.0f)));
    setLocalPosition(&wrenchEntity->transform, wrenchOffset);
    unsigned int pickingFBO, pickingRBO, pickingTexture;
    createPickingFBO(&pickingFBO, &pickingRBO, &pickingTexture, glm::ivec2(screenWidth, screenHeight));
    setFlags();
    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow |
                                   ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                   ImGuiTreeNodeFlags_SpanAvailWidth;

    unsigned char pixel[3];
    currentFrame = static_cast<float>(glfwGetTime());
    lastFrame = currentFrame;
    while (!glfwWindowShouldClose(window)) {
        currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        updateInput(window, &input);

        if (input.spawn && canSpawn) {
            canSpawn = false;
            Entity* newTrashCanEntity = createEntityFromModel(trashcan, trashcan->rootNode, &renderers, nullptr, &allColliders, true, true, &nextEntityID);
            entities.push_back(newTrashCanEntity);
            RigidBody* rb = addRigidBody(newTrashCanEntity, 10.0f, 5.0f, 25.0f, &rigidbodies);
            collider = (BoxCollider*)newTrashCanEntity->transform.children[0]->entity->components[component::kBoxCollider];
            collider->isActive = false;
            setPosition(&newTrashCanEntity->transform, getPosition(player->cameraController->camera->transform) + forward(player->cameraController->camera->transform));
            rb->linearVelocity = forward(player->cameraController->camera->transform) * 25.0f;
            rb->mass = 10.0f;
            rb->friction = 25.0f;
            rb->linearDrag = 3.0f;
        }

        if (!input.spawn) {
            canSpawn = true;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("ImGui");
        ImGui::SliderFloat("Move Speed", &player->moveSpeed, 0.0f, 45.0f);
        ImGui::InputFloat("jump height", &player->jumpHeight);
        ImGui::InputFloat("gravity", &gravity);
        ImGui::Checkbox("Enable Directional Light", &enableDirLight);
        if (enableDirLight) {
            ImGui::SliderFloat("Directional Light Brightness", &dirLightBrightness, 0.0f, 10.0f);
            ImGui::SliderFloat("Ambient Brightness", &ambientBrightness, 0.0f, 3.0f);
        }
        // ImGui::Image((ImTextureID)(intptr_t)pickingTexture, ImVec2(200, 200));
        for (Entity* entity : entities) {
            createImGuiEntityTree(entity, nodeFlags, &nodeClicked);
        }

        ImGui::End();

        sun.ambient = glm::vec3(ambientBrightness);
        sun.brightness = glm::vec3(dirLightBrightness);

        updatePlayer(window, &input, player);
        updateRigidBodies(rigidbodies, allColliders, gravity, deltaTime);
        for (Animator* animator : animators) {
            processAnimators(*animator, deltaTime, wrenchOffset);
        }
        updateCamera(player);
        setViewProjection(cameras[0]);
        glBindFramebuffer(GL_FRAMEBUFFER, pickingFBO);
        glViewport(0, 0, screenWidth, screenHeight);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        drawPickingScene(renderers, *cameras[0], pickingShader);

        if (isPicking) {
            glReadPixels(pickPosition.x, screenHeight - pickPosition.y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
            unsigned int id = pixel[0] + (pixel[1] << 8) + (pixel[2] << 16);
            bool foundEntity = false;
            for (Entity* entity : entities) {
                if (searchEntities(entity, id)) {
                    foundEntity = true;
                    break;
                }
            }

            if (!foundEntity) {
                nodeClicked = nullptr;
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, screenWidth, screenHeight);
        glClearColor(0.34, 0.34, 0.8, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        drawScene(renderers, *cameras[0], nodeClicked, enableDirLight, &sun);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    exitProgram(0);
    return 0;
}

bool searchEntities(Entity* entity, unsigned int id) {
    if (entity->id == id) {
        nodeClicked = entity;
        return true;
    }

    for (Transform* childEntity : entity->transform.children) {
        if (searchEntities(childEntity->entity, id)) {
            return true;
        }
    }

    return false;
}

void createImGuiEntityTree(Entity* entity, ImGuiTreeNodeFlags node_flags, Entity** node_clicked) {
    ImGui::PushID(entity);
    bool node_open = ImGui::TreeNodeEx(entity->name.c_str(), node_flags);

    if (ImGui::IsItemClicked()) {
        *node_clicked = entity;
    }

    if (node_open) {
        ImGui::Text("X: (%.1f), Y: (%.1f), Z: (%.1f)", getPosition(&entity->transform).x, getPosition(&entity->transform).y, getPosition(&entity->transform).z);

        for (Transform* child : entity->transform.children) {
            createImGuiEntityTree(child->entity, node_flags, node_clicked);
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
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
    player->cameraController->camera->aspectRatio = (float)screenWidth / screenHeight;
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