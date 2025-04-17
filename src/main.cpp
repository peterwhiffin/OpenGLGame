#define GLM_ENABLE_EXPERIMENTAL
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <glm/ext/quaternion_float.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <typeinfo>
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
Entity* createEntityFromModel(Model* model, Entity* root, ModelNode* parentNode, std::vector<Entity*>* entities, std::vector<MeshRenderer*>* renderers, std::vector<BoxCollider*>* colliders, Entity* parentEntity, glm::vec3 scale, glm::vec3 position, bool addCollider);
void updatePlayer(GLFWwindow* window, InputActions* input, Player* player, std::vector<BoxCollider*>& colliders);
void createImGuiEntityTree(Entity* entity, ImGuiTreeNodeFlags node_flags, Entity** node_clicked);
unsigned int getEntityID();
void drawScene(std::vector<MeshRenderer*>& renderers, Camera& camera);
void drawPickingScene(std::vector<MeshRenderer*>& renderers, Camera& camera);
void exitProgram(int code);
bool searchEntities(Entity* entity, unsigned int id);
bool checkAABB(const glm::vec3& centerA, const glm::vec3& extentA, const glm::vec3& centerB, const glm::vec3& extentB, glm::vec3& resolutionOut);
void updateRigidBodies(std::vector<RigidBody*>& rigidbodies, std::vector<BoxCollider*>& colliders);
void updateCamera(Player* player);
bool OBBvsOBB(const BoxCollider& a, const BoxCollider& b, glm::vec3& resolutionOut, glm::vec3& fullRes);
float ProjectOBB(const BoxCollider& box, const glm::vec3& axis);

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
bool enableDirLight = true;
float dirLightBrightness = 2.1f;
float ambientBrightness = 1.21f;
DirectionalLight sun;
unsigned int pickingShader;
bool isPicking = false;
glm::dvec2 pickPosition = glm::dvec2(0, 0);
glm::vec3 gunPosition = glm::vec3(0.0f, 0.0f, 0.0f);
Entity* gunEntity;
Entity* levelEntity;
RigidBody* trashcanRB;
float gravity = -18.81f;
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
    std::vector<BoxCollider*> dynamicColliders;
    std::vector<BoxCollider*> allColliders;
    std::vector<RigidBody*> rigidbodies;

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
    gunEntity = createEntityFromModel(m4Model, nullptr, m4Model->rootNode, &entities, &renderers, &allColliders, nullptr, glm::vec3(0.1f), glm::vec3(0.0f, 0.0f, 0.0f), false);
    levelEntity = createEntityFromModel(testRoom, nullptr, testRoom->rootNode, &entities, &renderers, &allColliders, nullptr, glm::vec3(1.0f), glm::vec3(0.0f, 0.0f, 0.0f), true);
    BoxCollider* gunCollider = new BoxCollider(gunEntity);
    gunCollider->center = glm::vec3(0.0f);
    gunCollider->extent = glm::vec3(0.5f, 0.4f, 0.2f);
    gunEntity->components.push_back(gunCollider);
    // allColliders.push_back(gunCollider);
    setPosition(gunEntity->transform, glm::vec3(2.0f, 5.0f, 2.0f));
    // gunCollider->center = getPosition(gunEntity->transform);
    RigidBody* gunRB = new RigidBody(gunEntity);
    gunEntity->components.push_back(gunRB);
    gunRB->collider = gunCollider;
    // allColliders.push_back(gunCollider);
    gunRB->collider->isActive = false;
    gunRB->linearVelocity = glm::vec3(0.0f);
    gunRB->angularVelocity = glm::vec3(0.0f);
    gunRB->linearDrag = 5.0f;
    gunRB->angularDrag = 0.5f;
    gunRB->mass = 5.0f;
    gunRB->friction = 80.0f;
    float w = gunRB->collider->extent.x * 2.0f;
    float h = gunRB->collider->extent.y * 2.0f;
    float d = gunRB->collider->extent.z * 2.0f;
    gunRB->momentOfInertia.x = (1.0f / 12.0f) * gunRB->mass * (h * h + d * d);
    gunRB->momentOfInertia.y = (1.0f / 12.0f) * gunRB->mass * (w * w + d * d);
    gunRB->momentOfInertia.z = (1.0f / 12.0f) * gunRB->mass * (w * w + h * h);
    rigidbodies.push_back(gunRB);

    for (int i = 0; i < levelEntity->children.size(); i++) {
        if (levelEntity->children[i]->name == "Trashcan_Base") {
            setPosition(levelEntity->children[i]->transform, glm::vec3(1.0f, 6.0f, 0.0f));
            BoxCollider* col = (BoxCollider*)levelEntity->children[i]->components[1];
            RigidBody* rb = new RigidBody(levelEntity->children[i]);
            trashcanRB = rb;
            rb->collider = col;
            rb->linearVelocity = glm::vec3(0.0f);
            rb->angularVelocity = glm::vec3(0.0f);
            rb->linearDrag = 5.0f;
            rb->angularDrag = 0.5f;
            rb->mass = 10.0f;
            rb->friction = 25.0f;
            rigidbodies.push_back(rb);
            rb->collider->isActive = false;
            float w = rb->collider->extent.x * 2.0f;
            float h = rb->collider->extent.y * 2.0f;
            float d = rb->collider->extent.z * 2.0f;
            rb->momentOfInertia.x = (1.0f / 12.0f) * rb->mass * (h * h + d * d);
            rb->momentOfInertia.y = (1.0f / 12.0f) * rb->mass * (w * w + d * d);
            rb->momentOfInertia.z = (1.0f / 12.0f) * rb->mass * (w * w + h * h);
            // col->center += glm::vec3(1.0f, 6.0f, 0.0f);
        }
    }

    Entity* playerEntity = new Entity();
    Entity* cameraTarget = new Entity();
    Entity* cameraEntity = new Entity();

    cameraTarget->name = "Camera Target";
    cameraEntity->name = "Camera";
    playerEntity->id = getEntityID();
    playerEntity->name = "player";
    Player* player = new Player(playerEntity);
    Camera camera(cameraEntity, glm::radians(68.0f), (float)screenWidth / screenHeight, 0.01, 10000);
    BoxCollider* playerCollider = new BoxCollider(playerEntity);
    RigidBody* playerRB = new RigidBody(playerEntity);
    playerRB->linearVelocity = glm::vec3(0.0f);
    playerRB->angularVelocity = glm::vec3(0.0f);
    playerRB->collider = playerCollider;
    playerRB->linearDrag = 0.0f;
    playerRB->angularDrag = 0.0f;
    playerRB->mass = 20.0f;
    playerRB->lockAngular = true;
    player->rigidbody = playerRB;
    rigidbodies.push_back(playerRB);
    playerCollider->center = glm::vec3(0.0f);
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
    // player->rigidbody->collider->center = getPosition(*player->rigidbody->transform);

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
    currentFrame = static_cast<float>(glfwGetTime());
    lastFrame = currentFrame;
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
        ImGui::Text("Velocity X: (%.1f), Velocity Y: (%.1f), Velocity Z: (%.1f)", player->rigidbody->linearVelocity.x, player->rigidbody->linearVelocity.y, player->rigidbody->linearVelocity.z);
        ImGui::Text("trash Velocity X: (%.1f), trash Velocity Y: (%.1f), trash Velocity Z: (%.1f)", trashcanRB->linearVelocity.x, trashcanRB->linearVelocity.y, trashcanRB->linearVelocity.z);

        ImGui::Text("m4 angular X: (%.1f), m4 angular Y: (%.1f), m4 angular Z: (%.1f)", gunRB->angularVelocity.x, gunRB->angularVelocity.y, gunRB->angularVelocity.z);
        ImGui::SliderFloat("Move Speed", &player->moveSpeed, 0.0f, 45.0f);
        ImGui::InputFloat("jump height", &player->jumpHeight);
        ImGui::InputFloat("gravity", &gravity);
        ImGui::Checkbox("Enable Demo Window", &enableDemoWindow);
        ImGui::Checkbox("Enable Directional Light", &enableDirLight);
        if (enableDirLight) {
            ImGui::SliderFloat("Directional Light Brightness", &dirLightBrightness, 0.0f, 10.0f);
            ImGui::SliderFloat("Ambient Brightness", &ambientBrightness, 0.0f, 3.0f);
        }
        // ImGui::Image((ImTextureID)(intptr_t)pickingTexture, ImVec2(200, 200));
        for (Entity* entity : entities) {
            createImGuiEntityTree(entity, nodeFlags, &nodeClicked);
        }

        if (enableDemoWindow) {
            ImGui::ShowDemoWindow();
        }
        ImGui::End();

        sun.ambient = glm::vec3(ambientBrightness);

        updatePlayer(window, &input, player, dynamicColliders);
        updateRigidBodies(rigidbodies, allColliders);
        updateCamera(player);
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

Entity* createEntityFromModel(Model* model, Entity* root, ModelNode* parentNode, std::vector<Entity*>* entities, std::vector<MeshRenderer*>* renderers, std::vector<BoxCollider*>* colliders, Entity* parentEntity, glm::vec3 scale, glm::vec3 position, bool addCollider) {
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

        setScale(childEntity->transform, scale);
        setPosition(childEntity->transform, position);
        setParent(childEntity->transform, parentEntity->transform);

        MeshRenderer* meshRenderer = new MeshRenderer(childEntity, &parentNode->children[i]->mesh);
        childEntity->components.push_back(meshRenderer);
        renderers->push_back(meshRenderer);

        if (addCollider) {
            BoxCollider* collider = new BoxCollider(childEntity);
            collider->center = meshRenderer->mesh->center;
            collider->extent = meshRenderer->mesh->extent;
            childEntity->components.push_back(collider);
            colliders->push_back(collider);
        }

        childEntity->id = getEntityID();
        childEntity->name = parentNode->children[i]->name;

        for (int j = 0; j < parentNode->children[i]->children.size(); j++) {
            createEntityFromModel(model, root, parentNode->children[i]->children[j], entities, renderers, colliders, childEntity, scale, position, addCollider);
        }
    }

    return root;
}

unsigned int getEntityID() {
    unsigned int id = nextEntityID;
    nextEntityID++;
    return id;
}

void updateRigidBodies(std::vector<RigidBody*>& rigidbodies, std::vector<BoxCollider*>& colliders) {
    const float epsilon = 1e-6f;
    glm::vec3 collisionResolution = glm::vec3(0.0f);
    glm::vec3 fullRes = glm::vec3(0.0f);

    for (RigidBody* rigidbody : rigidbodies) {
        rigidbody->linearVelocity.y += gravity * deltaTime;

        if (!rigidbody->lockAngular) {
            /* glm::vec3 centerOfMassWorld = getPosition(*rigidbody->transform) + getRotation(*rigidbody->transform) * rigidbody->collider->center;
            glm::vec3 torqueFromGravity = glm::cross(centerOfMassWorld - getPosition(*rigidbody->transform), glm::vec3(0, -rigidbody->mass * 18.81f, 0));
            rigidbody->angularVelocity += (torqueFromGravity / rigidbody->momentOfInertia) * deltaTime; */
        }
        glm::vec3 newPosition = getPosition(*rigidbody->transform) + rigidbody->linearVelocity * deltaTime;
        float angle = glm::length(rigidbody->angularVelocity);
        if (!rigidbody->lockAngular && angle > epsilon) {
            glm::vec3 axis = glm::normalize(rigidbody->angularVelocity);
            glm::quat deltaRot = glm::angleAxis(angle * deltaTime, axis);
            glm::quat currentRot = rigidbody->transform->rotation;
            glm::quat newRot = glm::normalize(deltaRot * currentRot);

            // glm::vec3 newRotation = glm::eulerAngles(getRotation(*rigidbody->transform)) + rigidbody->angularVelocity;

            // setLocalRotation(*rigidbody->transform, newRot);

            setRotation(*rigidbody->transform, newRot);
        }
        setPosition(*rigidbody->transform, newPosition);
        // rigidbody->collider->center = getPosition(*rigidbody->transform);
        rigidbody->collider->axes[0] = right(rigidbody->transform);
        rigidbody->collider->axes[1] = up(rigidbody->transform);
        rigidbody->collider->axes[2] = forward(rigidbody->transform);
    }

    for (int i = 0; i < rigidbodies.size(); i++) {
        RigidBody* rigidbody = rigidbodies[i];
        for (int j = i + 1; j < rigidbodies.size(); j++) {
            RigidBody* rb = rigidbodies[j];

            /*     rb->collider->axes[0] = right(rb->collider->transform);
                rb->collider->axes[1] = up(rb->collider->transform);
                rb->collider->axes[2] = forward(rb->collider->transform);
     */
            if (OBBvsOBB(*rigidbody->collider, *rb->collider, collisionResolution, fullRes)) {
                setPosition(*rigidbody->transform, getPosition(*rigidbody->transform) - (collisionResolution / 2.0f));
                setPosition(*rb->transform, getPosition(*rb->transform) + (collisionResolution / 2.0f));

                glm::vec3 contactPoint = rb->collider->center + collisionResolution;
                contactPoint = fullRes;

                glm::vec3 forceDirection = glm::normalize(rigidbody->linearVelocity - rb->linearVelocity);
                glm::vec3 normalized = forceDirection;
                normalized.y = 0.0f;
                float totalMass = rb->mass + rigidbody->mass;
                float rbSpeed = glm::length(rb->linearVelocity);
                float rigidbodySpeed = glm::length(rigidbody->linearVelocity);

                if (collisionResolution.y != 0.0f) {
                    // rigidbody->linearVelocity.y = 0.0f;
                    // rb->linearVelocity.y = 0.0f;
                }
                float velocityDiff = glm::abs(rbSpeed - rigidbodySpeed);
                velocityDiff *= 0.8f;
                float rbMassPart = rigidbody->mass / totalMass;
                float rigidbodyMassPart = rb->mass / totalMass;

                glm::vec3 force = normalized * rbMassPart * velocityDiff;
                glm::vec3 force2 = normalized * rigidbodyMassPart * velocityDiff;
                glm::vec3 torque = glm::cross(contactPoint - rigidbody->collider->center, force);
                glm::vec3 torque2 = glm::cross(contactPoint - rb->collider->center, force2);

                rb->angularVelocity += (torque2 / rb->momentOfInertia) * deltaTime;
                rb->linearVelocity += force;
                rigidbody->linearVelocity -= force2;
                rigidbody->angularVelocity += (torque / rigidbody->momentOfInertia) * deltaTime;

                float magnitude = glm::length(rigidbody->linearVelocity);

                if (magnitude > epsilon) {
                    float newMagnitude = glm::max(magnitude - rigidbody->friction * deltaTime, 0.0f);
                    glm::vec3 normalVelocity = glm::normalize(rigidbody->linearVelocity);
                    rigidbody->linearVelocity = normalVelocity * newMagnitude;
                } else {
                    rigidbody->linearVelocity = glm::vec3(0.0f);
                }
            }
        }

        for (BoxCollider* collider : colliders) {
            if (collider == rigidbody->collider || !collider->isActive) {
                continue;
            }

            collider->axes[0] = right(collider->transform);
            collider->axes[1] = up(collider->transform);
            collider->axes[2] = forward(collider->transform);
            // collider->center = collider->transform->position;

            if (OBBvsOBB(*rigidbody->collider, *collider, collisionResolution, fullRes)) {
                setPosition(*rigidbody->transform, getPosition(*rigidbody->transform) - collisionResolution);
                // rigidbody->collider->center = getPosition(*rigidbody->transform);

                glm::vec3 contactPoint = fullRes;

                float magnitude = glm::length(rigidbody->linearVelocity);

                if (magnitude > epsilon) {
                    float newMagnitude = glm::max(magnitude - rigidbody->friction * deltaTime, 0.0f);
                    glm::vec3 normalVelocity = glm::normalize(rigidbody->linearVelocity);
                    rigidbody->linearVelocity = normalVelocity * newMagnitude;
                } else {
                    rigidbody->linearVelocity = glm::vec3(0.0f);
                }

                if (collisionResolution.y != 0.0f) {
                    // rigidbody->linearVelocity.y = 0.0f;
                }
                glm::vec3 forceDirection = -rigidbody->linearVelocity;
                glm::vec3 torque = glm::cross(contactPoint - getPosition(*rigidbody->transform) + getRotation(*rigidbody->transform) * rigidbody->collider->center, forceDirection);
                rigidbody->angularVelocity += (torque / rigidbody->momentOfInertia) * deltaTime;
            }
        }

        float linearMagnitude = glm::length(rigidbody->linearVelocity);
        float angularMagnitude = glm::length(rigidbody->angularVelocity);

        if (linearMagnitude > epsilon) {
            glm::vec3 normalVel = glm::normalize(rigidbody->linearVelocity);
            linearMagnitude = glm::max(linearMagnitude - (rigidbody->linearDrag * deltaTime), 0.0f);
            rigidbody->linearVelocity = normalVel * linearMagnitude;
        } else {
            rigidbody->linearVelocity = glm::vec3(0.0f);
        }

        if (angularMagnitude > epsilon) {
            glm::vec3 normalVel = glm::normalize(rigidbody->angularVelocity);
            angularMagnitude = glm::max(angularMagnitude - rigidbody->angularDrag * deltaTime, 0.0f);
            rigidbody->angularVelocity = normalVel * angularMagnitude;
        } else {
            rigidbody->angularVelocity = glm::vec3(0.0f);
        }
    }
}

float ProjectOBB(const BoxCollider& box, const glm::vec3& axis) {
    return box.extent.x * std::abs(glm::dot(axis, box.axes[0])) +
           box.extent.y * std::abs(glm::dot(axis, box.axes[1])) +
           box.extent.z * std::abs(glm::dot(axis, box.axes[2]));
}

bool OBBvsOBB(const BoxCollider& a, const BoxCollider& b, glm::vec3& resolutionOut, glm::vec3& fullRes) {
    const float epsilon = 1e-6f;
    float minOverlap = std::numeric_limits<float>::max();
    glm::vec3 smallestAxis;

    glm::vec3 axesToTest[15];
    int axisCount = 0;

    // Add A's local axes
    for (int i = 0; i < 3; ++i) axesToTest[axisCount++] = a.axes[i];
    // Add B's local axes
    for (int i = 0; i < 3; ++i) axesToTest[axisCount++] = b.axes[i];
    // Add cross products
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            glm::vec3 axis = glm::cross(a.axes[i], b.axes[j]);
            if (glm::dot(axis, axis) > epsilon) {
                axesToTest[axisCount++] = glm::normalize(axis);
            }
        }
    }

    glm::vec3 delta = (getPosition(*b.transform) + getRotation(*b.transform) * b.center) - (getPosition(*a.transform) + getRotation(*a.transform) * a.center);

    fullRes = glm::vec3(0.0f);
    for (int i = 0; i < axisCount; ++i) {
        const glm::vec3& axis = axesToTest[i];
        float dist = std::abs(glm::dot(delta, axis));
        float projA = ProjectOBB(a, axis);
        float projB = ProjectOBB(b, axis);
        float overlap = projA + projB - dist;

        if (overlap < 0.0f) {
            // Separating axis found
            resolutionOut = glm::vec3(0.0f);
            fullRes = glm::vec3(0.0f);
            return false;
        }

        if (overlap < minOverlap) {
            minOverlap = overlap;
            smallestAxis = axis;

            // Ensure resolution pushes A away from B
            if (glm::dot(delta, axis) < 0.0f) {
                smallestAxis = -axis;
                fullRes += -axis * overlap;
            } else {
                fullRes += axis * overlap;
            }
        }
    }

    glm::vec3 centerA = getPosition(*a.transform) + a.transform->rotation * a.center;
    glm::vec3 centerB = getPosition(*b.transform) + b.transform->rotation * b.center;
    fullRes = centerA + glm::dot(centerB - centerA, smallestAxis) * smallestAxis;
    resolutionOut = smallestAxis * minOverlap;
    return true;
}

void updatePlayer(GLFWwindow* window, InputActions* input, Player* player, std::vector<BoxCollider*>& colliders) {
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

    finalMove.y = player->rigidbody->linearVelocity.y;
    if (player->isGrounded) {
        if (input->jump) {
            finalMove.y = player->jumpHeight;
        }
    }

    player->isGrounded = true;
    player->rigidbody->linearVelocity = finalMove;
}

void updateCamera(Player* player) {
    setPosition(*player->cameraController->camera->transform, glm::mix(getPosition(*player->cameraController->camera->transform), getPosition(*player->cameraController->cameraTarget), 0.9f));
    setRotation(*player->cameraController->camera->transform, glm::slerp(getRotation(*player->cameraController->camera->transform), getRotation(*player->cameraController->cameraTarget), 0.9f));
}

bool checkAABB(const glm::vec3& centerA, const glm::vec3& extentA, const glm::vec3& centerB, const glm::vec3& extentB, glm::vec3& resolutionOut) {
    glm::vec3 delta = centerA - centerB;
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