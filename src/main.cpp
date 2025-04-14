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
void createEntityFromModel(Model* model, ModelNode* parentNode, std::vector<Entity*>* entities, std::vector<MeshRenderer*>* renderers, Entity* parentEntity, glm::vec3 scale, glm::vec3 position);
void createImGuiEntityTree(Entity* entity, ImGuiTreeNodeFlags node_flags, Entity** node_clicked);
unsigned int getEntityID();
void drawScene(std::vector<MeshRenderer*>& renderers, Camera& camera);
void drawPickingScene(std::vector<MeshRenderer*>& renderers, Camera& camera);
void exitProgram(int code);
bool searchEntities(Entity* entity, unsigned int id);

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

    Texture white;
    Texture black;
    white.id = whiteTexture;
    black.id = blackTexture;
    white.path = "white";
    black.path = "black";
    allTextures.push_back(white);
    allTextures.push_back(black);

    // Model* sponzaModel = loadModel("../resources/models/sponza/sponza.obj", &allTextures, defaultShader);
    // Model* m4Model = loadModel("../resources/models/M4/ddm4 v7.obj", &allTextures, defaultShader);
    Model* testRoom = loadModel("../resources/models/testroom/testroom.obj", &allTextures, defaultShader);

    // createEntityFromModel(sponzaModel, sponzaModel->rootNode, &entities, &renderers, nullptr, glm::vec3(0.01f, 0.01f, 0.01f), glm::vec3(0.0f));
    // createEntityFromModel(m4Model, m4Model->rootNode, &entities, &renderers, nullptr, glm::vec3(0.1f), glm::vec3(0.0f, 15.0f, 0.0f));
    createEntityFromModel(testRoom, testRoom->rootNode, &entities, &renderers, nullptr, glm::vec3(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

    Entity* playerEntity = new Entity();
    Camera camera(playerEntity, glm::radians(68.0f), (float)screenWidth / screenHeight, 0.1, 10000);
    mainCamera = &camera;
    CameraController cameraController(playerEntity, camera);

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
                                   ImGuiTreeNodeFlags_SpanAvailWidth |
                                   ImGuiTreeNodeFlags_DefaultOpen;

    unsigned char pixel[3];
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
        ImGui::Text("Cursor: (%.1f, %.1f)", input.cursorPosition.x, input.cursorPosition.y);
        ImGui::Checkbox("Enable Demo Window", &enableDemoWindow);
        ImGui::Checkbox("Enable Directional Light", &enableDirLight);
        ImGui::SliderFloat("Directional Light Brightness", &dirLightBrightness, 0.0f, 10.0f);
        ImGui::SliderFloat("Ambient Brightness", &ambientBrightness, 0.0f, 3.0f);
        ImGui::SliderFloat("Move Speed", &cameraController.moveSpeed, 0.0f, 25.0f);
        ImGui::Image((ImTextureID)(intptr_t)pickingTexture, ImVec2(200, 200));
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
        glm::mat4 model = glm::translate(glm::mat4(1.0f), renderer->transform->position);
        model *= glm::mat4_cast(renderer->transform->rotation);
        model = glm::scale(model, renderer->transform->scale);

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
        glm::mat4 model = glm::translate(glm::mat4(1.0f), renderer->transform->position);
        model *= glm::mat4_cast(renderer->transform->rotation);
        model = glm::scale(model, renderer->transform->scale);
        model = renderer->transform->localToWorldMatrix;
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
            // glUniform3fv(glGetUniformLocation(shader, "dirLight.specular"), 1, glm::value_ptr(sun.specular * dirLightBrightness));
            glUniform3fv(glGetUniformLocation(shader, "dirLight.specular"), 1, glm::value_ptr(sun.specular * dirLightBrightness));
            // std::cout << "spec map: " << subMesh->material.textures[1].path << std::endl;
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
        for (Entity* child : entity->children) {
            createImGuiEntityTree(child, node_flags, node_clicked);
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void createEntityFromModel(Model* model, ModelNode* parentNode, std::vector<Entity*>* entities, std::vector<MeshRenderer*>* renderers, Entity* parentEntity, glm::vec3 scale, glm::vec3 position) {
    if (parentEntity == nullptr) {
        Entity* rootEntity = new Entity();
        rootEntity->name = model->name;
        rootEntity->id = getEntityID();
        parentEntity = rootEntity;
        entities->push_back(parentEntity);
    }

    for (int i = 0; i < parentNode->children.size(); i++) {
        Entity* childEntity = new Entity();

        counter++;
        MeshRenderer* meshRenderer = new MeshRenderer(childEntity, &parentNode->children[i]->mesh);
        renderers->push_back(meshRenderer);
        childEntity->id = getEntityID();
        childEntity->name = parentNode->children[i]->name;
        childEntity->parent = parentEntity;
        setScale(childEntity->transform, scale);
        // childEntity->transform.scale = scale;
        childEntity->transform.position = parentNode->children[i]->transform[3];
        // childEntity->transform.position += position;

        setPosition(childEntity->transform, position);
        setParent(childEntity->transform, parentEntity->transform);
        // parentEntity->children.push_back(childEntity);

        for (int j = 0; j < parentNode->children[i]->children.size(); j++) {
            createEntityFromModel(model, parentNode->children[i]->children[j], entities, renderers, childEntity, scale, position);
        }
    }
}

unsigned int getEntityID() {
    unsigned int id = nextEntityID;
    nextEntityID++;
    return id;
}

void updateCamera(GLFWwindow* window, InputActions* input, CameraController* camera) {
    if (!input->altFire) {
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