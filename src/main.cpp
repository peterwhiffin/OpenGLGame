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
void setViewProjection(Camera* camera);
void onScreenChanged(GLFWwindow* window, int width, int height);
void initializeIMGUI(GLFWwindow* window);
Entity* createEntityFromModel(Model* model, Entity* root, ModelNode* parentNode, std::vector<Entity*>* entities, std::vector<MeshRenderer*>* renderers, std::vector<BoxCollider*>* colliders, Entity* parentEntity, glm::vec3 scale, glm::vec3 position);
void updateCamera(GLFWwindow* window, InputActions* input, Player* player, std::vector<BoxCollider*>& colliders);
void createImGuiEntityTree(Entity* entity, ImGuiTreeNodeFlags node_flags, Entity** node_clicked);
unsigned int getEntityID();
void drawScene(std::vector<MeshRenderer*>& renderers, Camera& camera);
void drawPickingScene(std::vector<MeshRenderer*>& renderers, Camera& camera);
void exitProgram(int code);
bool searchEntities(Entity* entity, unsigned int id);
bool checkAABB(const glm::vec3& centerA, const glm::vec3& extentA, const glm::vec3& centerB, const glm::vec3& extentB, glm::vec3& resolutionOut);
Entity* m4;
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
unsigned int nextEntityID = 1000000;
Entity* nodeClicked = nullptr;
bool enableDemoWindow = false;
bool enableDirLight = false;
float dirLightBrightness = 1.0f;
float ambientBrightness = 0.21f;
DirectionalLight sun;
unsigned int pickingShader;
bool isPicking = false;
glm::dvec2 pickPosition = glm::dvec2(0, 0);
glm::vec3 gunPosition = glm::vec3(0.0f, 0.0f, 0.0f);
// Entity* m4Entity;
Entity* levelEntity;
float gravity = -9.81f;
float yVelocity = 0.0f;
float terminalVelocity = 56.0f;
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
    std::vector<BoxCollider*> colliders;

    Texture white;
    Texture black;
    white.id = whiteTexture;
    black.id = blackTexture;
    white.path = "white";
    black.path = "black";
    allTextures.push_back(white);
    allTextures.push_back(black);

    // Model* sponzaModel = loadModel("../resources/models/sponza/sponza.obj", &allTextures, defaultShader);
    Model* m4Model = loadModel("../resources/models/M4/ddm4 v7.obj", &allTextures, defaultShader);
    Model* testRoom = loadModel("../resources/models/testroom/testroom.obj", &allTextures, defaultShader);

    // createEntityFromModel(sponzaModel, sponzaModel->rootNode, &entities, &renderers, nullptr, glm::vec3(0.01f, 0.01f, 0.01f), glm::vec3(0.0f));
    // m4Entity = createEntityFromModel(m4Model, nullptr, m4Model->rootNode, &entities, &renderers, &colliders, nullptr, glm::vec3(0.1f), glm::vec3(0.0f, 0.0f, 0.0f));
    levelEntity = createEntityFromModel(testRoom, nullptr, testRoom->rootNode, &entities, &renderers, &colliders, nullptr, glm::vec3(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

    Entity* playerEntity = new Entity();
    Entity* cameraTarget = new Entity();
    Entity* cameraEntity = new Entity();

    cameraTarget->name = "Camera Target";
    cameraEntity->name = "Camera";
    playerEntity->id = getEntityID();
    playerEntity->name = "Player";
    Player* player = new Player(playerEntity);
    Camera camera(cameraEntity, glm::radians(68.0f), (float)screenWidth / screenHeight, 0.01, 10000);
    BoxCollider* playerCollider = new BoxCollider(playerEntity);
    playerCollider->center = getPosition(playerEntity->transform);
    playerCollider->extent = glm::vec3(0.25f, 0.9f, 0.25f);
    player->collider = playerCollider;
    mainCamera = &camera;

    CameraController cameraController(playerEntity, &camera);
    cameraController.cameraTarget = &cameraTarget->transform;

    entities.push_back(cameraTarget);
    entities.push_back(playerEntity);
    entities.push_back(cameraEntity);
    player->cameraController = &cameraController;
    playerEntity->components.push_back(&cameraController);
    playerEntity->components.push_back(player);
    // setParent(m4Entity->transform, playerEntity->transform);
    setParent(cameraTarget->transform, playerEntity->transform);
    setPosition(playerEntity->transform, glm::vec3(0.0f, 3.0f, 0.0f));
    setLocalPosition(cameraTarget->transform, glm::vec3(0.0f, 0.7f, 0.0f));

    unsigned int pickingFBO;
    unsigned int pickingRBO;
    unsigned int pickingTexture;

    glGenFramebuffers(1, &pickingFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, pickingFBO);

    glGenTextures(1, &pickingTexture);
    glBindTexture(GL_TEXTURE_2D, pickingTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenWidth, screenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pickingTexture, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenRenderbuffers(1, &pickingRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, pickingRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, screenWidth, screenHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, pickingRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow |
                                   ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                   ImGuiTreeNodeFlags_SpanAvailWidth;

    unsigned char pixel[3];
    glm::vec3 gunOffset = glm::vec3(0.0f, -0.09f, -0.2f);
    glm::vec3 gunLocalRot = glm::vec3(0.0f, glm::radians(90.0f), 0.0f);
    // setPosition(m4Entity->transform, glm::vec3(0.0f, 2.0f, 0.0f));
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
        ImGui::InputFloat("gun offset X", &gunOffset.x);
        ImGui::InputFloat("gun offset Y", &gunOffset.y);
        ImGui::InputFloat("gun offset Z", &gunOffset.z);
        ImGui::InputFloat("gun rot X", &gunLocalRot.x);
        ImGui::InputFloat("gun rot Y", &gunLocalRot.y);
        ImGui::InputFloat("gun rot Z", &gunLocalRot.z);
        // ImGui::Text("GunPosX: (%.1f)", getPosition(m4Entity->transform).x);
        // ImGui::Text("GunPosY: (%.1f)", getPosition(m4Entity->transform).y);
        // ImGui::Text("GunPosZ: (%.1f)", getPosition(m4Entity->transform).z);
        ImGui::Text("Cursor: (%.1f, %.1f)", input.cursorPosition.x, input.cursorPosition.y);
        ImGui::Checkbox("Enable Demo Window", &enableDemoWindow);
        ImGui::Checkbox("Enable Directional Light", &enableDirLight);
        ImGui::SliderFloat("Directional Light Brightness", &dirLightBrightness, 0.0f, 10.0f);
        ImGui::SliderFloat("Ambient Brightness", &ambientBrightness, 0.0f, 3.0f);
        ImGui::SliderFloat("Move Speed", &cameraController.moveSpeed, 0.0f, 25.0f);
        // ImGui::Image((ImTextureID)(intptr_t)pickingTexture, ImVec2(200, 200));
        for (Entity* entity : entities) {
            createImGuiEntityTree(entity, nodeFlags, &nodeClicked);
        }

        if (enableDemoWindow) {
            ImGui::ShowDemoWindow();
        }
        ImGui::End();

        sun.ambient = glm::vec3(ambientBrightness);

        // setLocalPosition(m4Entity->transform, gunOffset);
        // setLocalRotation(m4Entity->transform, glm::quat(glm::radians(gunLocalRot)));
        updateCamera(window, &input, player, colliders);
        setViewProjection(&camera);
        glBindFramebuffer(GL_FRAMEBUFFER, pickingFBO);
        glViewport(0, 0, screenWidth, screenHeight);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        drawPickingScene(renderers, camera);

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

        drawScene(renderers, camera);
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

    for (Entity* childEntity : entity->children) {
        if (searchEntities(childEntity, id)) {
            return true;
        }
    }

    return false;
}

void drawPickingScene(std::vector<MeshRenderer*>& renderers, Camera& camera) {
    for (MeshRenderer* renderer : renderers) {
        glm::mat4 model = renderer->transform->localToWorldMatrix;
        glBindVertexArray(renderer->mesh->VAO);

        for (SubMesh* subMesh : renderer->mesh->subMeshes) {
            unsigned char r = renderer->entity->id & 0xFF;
            unsigned char g = (renderer->entity->id >> 8) & 0xFF;
            unsigned char b = (renderer->entity->id >> 16) & 0xFF;
            glm::vec3 idColor = glm::vec3(r, g, b) / 255.0f;

            glUseProgram(pickingShader);
            glUniformMatrix4fv(uniform_location::kModelMatrix, 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix4fv(uniform_location::kViewMatrix, 1, GL_FALSE, glm::value_ptr(camera.viewMatrix));
            glUniformMatrix4fv(uniform_location::kProjectionMatrix, 1, GL_FALSE, glm::value_ptr(camera.projectionMatrix));
            glUniform3fv(uniform_location::kBaseColor, 1, glm::value_ptr(idColor));

            glDrawElements(GL_TRIANGLES, subMesh->indexCount, GL_UNSIGNED_INT, (void*)(subMesh->indexOffset * sizeof(unsigned int)));
        }

        glBindVertexArray(0);
    }
}

void drawScene(std::vector<MeshRenderer*>& renderers, Camera& camera) {
    for (MeshRenderer* renderer : renderers) {
        glm::mat4 model = renderer->transform->localToWorldMatrix;
        glm::mat4 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));

        glBindVertexArray(renderer->mesh->VAO);

        for (SubMesh* subMesh : renderer->mesh->subMeshes) {
            unsigned int shader = subMesh->material.shader;
            glm::vec4 baseColor = (renderer->entity == nodeClicked) ? glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) : subMesh->material.baseColor;

            glUseProgram(shader);
            glUniformMatrix4fv(uniform_location::kModelMatrix, 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix4fv(uniform_location::kViewMatrix, 1, GL_FALSE, glm::value_ptr(camera.viewMatrix));
            glUniformMatrix4fv(uniform_location::kProjectionMatrix, 1, GL_FALSE, glm::value_ptr(camera.projectionMatrix));
            glUniformMatrix4fv(uniform_location::kNormalMatrix, 1, GL_FALSE, glm::value_ptr(normalMatrix));
            glUniform4fv(uniform_location::kBaseColor, 1, glm::value_ptr(baseColor));
            // glUniform1f(uniform_location::kShininess, subMesh->material.shininess);
            glUniform1f(uniform_location::kShininess, 512.0f);
            glUniform3fv(uniform_location::kViewPos, 1, glm::value_ptr(camera.transform->position));

            glUniform1i(glGetUniformLocation(shader, "dirLight.enabled"), enableDirLight);
            glUniform3fv(glGetUniformLocation(shader, "dirLight.ambient"), 1, glm::value_ptr(sun.ambient));
            glUniform3fv(glGetUniformLocation(shader, "dirLight.diffuse"), 1, glm::value_ptr(sun.diffuse * dirLightBrightness));
            glUniform3fv(glGetUniformLocation(shader, "dirLight.specular"), 1, glm::value_ptr(sun.specular * dirLightBrightness));
            glActiveTexture(GL_TEXTURE0 + uniform_location::kTextureDiffuseUnit);
            glBindTexture(GL_TEXTURE_2D, subMesh->material.textures[0].id);
            glActiveTexture(GL_TEXTURE0 + uniform_location::kTextureSpecularUnit);
            glBindTexture(GL_TEXTURE_2D, subMesh->material.textures[1].id);
            glDrawElements(GL_TRIANGLES, subMesh->indexCount, GL_UNSIGNED_INT, (void*)(subMesh->indexOffset * sizeof(unsigned int)));
        }

        glBindVertexArray(0);
    }
}

void createImGuiEntityTree(Entity* entity, ImGuiTreeNodeFlags node_flags, Entity** node_clicked) {
    ImGui::PushID(entity);
    bool node_open = ImGui::TreeNodeEx(entity->name.c_str(), node_flags);

    if (ImGui::IsItemClicked()) {
        *node_clicked = entity;
    }

    if (node_open) {
        ImGui::Text("X: (%.1f), Y: (%.1f), Z: (%.1f)", getPosition(entity->transform).x, getPosition(entity->transform).y, getPosition(entity->transform).z);

        for (Entity* child : entity->children) {
            createImGuiEntityTree(child, node_flags, node_clicked);
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}

Entity* createEntityFromModel(Model* model, Entity* root, ModelNode* parentNode, std::vector<Entity*>* entities, std::vector<MeshRenderer*>* renderers, std::vector<BoxCollider*>* colliders, Entity* parentEntity, glm::vec3 scale, glm::vec3 position) {
    if (parentEntity == nullptr) {
        Entity* rootEntity = new Entity();
        rootEntity->name = model->name;
        rootEntity->id = getEntityID();
        parentEntity = rootEntity;
        entities->push_back(parentEntity);
        root = rootEntity;
    }

    for (int i = 0; i < parentNode->children.size(); i++) {
        Entity* childEntity = new Entity();

        counter++;
        MeshRenderer* meshRenderer = new MeshRenderer(childEntity, &parentNode->children[i]->mesh);
        BoxCollider* collider = new BoxCollider(childEntity);
        collider->center = meshRenderer->mesh->center;
        collider->extent = meshRenderer->mesh->extent;
        childEntity->components.push_back(collider);
        childEntity->components.push_back(meshRenderer);

        colliders->push_back(collider);
        renderers->push_back(meshRenderer);
        childEntity->id = getEntityID();
        childEntity->name = parentNode->children[i]->name;

        setScale(childEntity->transform, scale);
        setPosition(childEntity->transform, position);
        setParent(childEntity->transform, parentEntity->transform);

        collider->center = getPosition(childEntity->transform) + meshRenderer->mesh->center;

        for (int j = 0; j < parentNode->children[i]->children.size(); j++) {
            createEntityFromModel(model, root, parentNode->children[i]->children[j], entities, renderers, colliders, childEntity, scale, position);
        }
    }

    return root;
}

unsigned int getEntityID() {
    unsigned int id = nextEntityID;
    nextEntityID++;
    return id;
}

void updateCamera(GLFWwindow* window, InputActions* input, Player* player, std::vector<BoxCollider*>& colliders) {
    /*   if (!input->altFire) {
          glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

          if (input->fire && !isPicking) {
              isPicking = true;
              glfwGetCursorPos(window, &pickPosition.x, &pickPosition.y);
          }

          if (!input->fire && isPicking) {
              isPicking = false;
          }
          return;
      }
   */
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    float xOffset = input->lookX * player->cameraController->sensitivity;
    float yOffset = input->lookY * player->cameraController->sensitivity;
    float pitch = player->cameraController->pitch;
    float yaw = player->cameraController->yaw;

    yaw -= xOffset;
    pitch -= yOffset;

    if (pitch > 89.0f) {
        pitch = 89.0f;
    }

    if (pitch < -89.0f) {
        pitch = -89.0f;
    }

    player->cameraController->pitch = pitch;
    player->cameraController->yaw = yaw;

    float upTest = 0.0f;
    if (input->jump) {
        upTest = 1.0f;
    }

    glm::vec3 cameraTargetRotation = glm::vec3(glm::radians(player->cameraController->pitch), 0.0f, 0.0f);
    glm::vec3 playerRotation = glm::vec3(0.0f, glm::radians(player->cameraController->yaw), 0.0f);

    setLocalRotation(*player->cameraController->cameraTarget, glm::quat(cameraTargetRotation));
    setRotation(*player->transform, glm::quat(playerRotation));

    glm::vec3 moveDir = glm::vec3(0.0f);
    moveDir += input->movement.y * forward(player->transform) + input->movement.x * right(player->transform);
    glm::vec3 finalMove = moveDir * player->moveSpeed;

    if (player->isGrounded) {
        if (input->jump) {
            yVelocity = 5.0f;
        }
    }

    player->isGrounded = false;

    finalMove.y += yVelocity;
    glm::vec3 oldPos = getPosition(*player->transform);
    setPosition(*player->transform, player->transform->position + finalMove * deltaTime);

    glm::vec3 collisionResolution = glm::vec3(0.0f);
    player->collider->center = getPosition(*player->transform);
    // player->collider->extent = glm::vec3(0.25f, 0.9f, 0.25f);
    for (BoxCollider* collider : colliders) {
        if (collider == player->collider) {
            continue;
        }

        if (checkAABB(player->collider->center, player->collider->extent, collider->center, collider->extent, collisionResolution)) {
            // setPosition(*player->transform, oldPos + ((finalMove * deltaTime) - collisionResolution));
            setPosition(*player->transform, getPosition(*player->transform) - collisionResolution);
            player->collider->center = getPosition(*player->transform);
            // player->collider->extent = glm::vec3(0.25f, 0.9f, 0.25f);

            // glm::vec3 newDir = oldPos + collisionResolution + moveDir - collisionResolution;
            // setPosition(*player->transform, oldPos);
            if (collisionResolution.y != 0.0f) {
                player->isGrounded = true;
            }
            // yVelocity = 0.0f;
        }
    }

    if (!player->isGrounded) {
        yVelocity += gravity * deltaTime;
        yVelocity = glm::min(yVelocity, terminalVelocity);
    } else {
        yVelocity = 0.0f;
    }

    setPosition(*player->cameraController->camera->transform, getPosition(*player->cameraController->cameraTarget));
    setRotation(*player->cameraController->camera->transform, getRotation(*player->cameraController->cameraTarget));
}

bool checkAABB(const glm::vec3& centerA, const glm::vec3& extentA, const glm::vec3& centerB, const glm::vec3& extentB, glm::vec3& resolutionOut) {
    glm::vec3 delta = centerB - centerA;
    glm::vec3 overlap = extentA + extentB - glm::abs(delta);

    resolutionOut = glm::vec3(0.0f);

    if (overlap.x <= 0.0f || overlap.y <= 0.0f || overlap.z <= 0.0f) {
        return false;
    }

    float minOverlap = overlap.x;
    float push = delta.x < 0 ? -1.0f : 1.0f;
    glm::vec3 pushDir = glm::vec3(push, 0.0f, 0.0f);

    if (overlap.y < minOverlap) {
        minOverlap = overlap.y;
        push = delta.y < 0 ? -1.0f : 1.0f;
        pushDir = glm::vec3(0.0f, push, 0.0f);
    }

    if (overlap.z < minOverlap) {
        minOverlap = overlap.z;
        push = delta.z < 0 ? -1.0f : 1.0f;
        pushDir = glm::vec3(0.0f, 0.0f, push);
    }

    resolutionOut = pushDir * minOverlap;
    return true;
}

void checkGround() {
}

void setViewProjection(Camera* camera) {
    glm::vec3 position = getPosition(*camera->transform);
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