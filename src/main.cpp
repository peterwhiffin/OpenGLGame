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
#include "debug.h"

GLFWwindow* createContext(WindowData* windowData);
void onScreenChanged(GLFWwindow* window, int width, int height);
void initializeIMGUI(GLFWwindow* window);
void exitProgram(int code);

int main() {
    Scene* scene = new Scene();
    WindowData windowData;
    windowData.width = 800;
    windowData.height = 600;

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

    bool enableDirLight = true;
    bool isPicking = false;
    bool canSpawn = true;

    GLFWwindow* window = createContext(&windowData);
    InputActions input = InputActions();

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

    trashCanEntity = createEntityFromModel(&entities, trashcan, trashcan->rootNode, &renderers, nullptr, &allColliders, true, true, &nextEntityID);
    levelEntity = createEntityFromModel(&entities, testRoom, testRoom->rootNode, &renderers, nullptr, &allColliders, true, true, &nextEntityID);
    wrenchEntity = createEntityFromModel(&entities, wrench, wrench->rootNode, &renderers, nullptr, &allColliders, true, false, &nextEntityID);

    addAnimator(wrenchEntity, wrench, &animators);
    addRigidBody(trashCanEntity, 10.0f, 5.0f, 25.0f, &rigidbodies);

    setPosition(&trashCanEntity->transform, glm::vec3(1.0f, 3.0f, 2.0f));
    BoxCollider* collider = (BoxCollider*)trashCanEntity->transform.children[0]->entity->components[component::kBoxCollider];
    collider->isActive = false;

    player = createPlayer(&nextEntityID, (float)windowData.width / windowData.height, &entities, &allColliders, &rigidbodies, &cameras);
    setParent(&wrenchEntity->transform, player->cameraController->cameraTarget);
    setLocalRotation(&wrenchEntity->transform, glm::quat(glm::vec3(glm::radians(0.0f), glm::radians(180.0f), 0.0f)));
    setLocalPosition(&wrenchEntity->transform, wrenchOffset);
    unsigned int pickingFBO, pickingRBO, pickingTexture;
    createPickingFBO(&pickingFBO, &pickingRBO, &pickingTexture, &windowData);
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
            Entity* newTrashCanEntity = createEntityFromModel(&entities, trashcan, trashcan->rootNode, &renderers, nullptr, &allColliders, true, true, &nextEntityID);
            RigidBody* rb = addRigidBody(newTrashCanEntity, 10.0f, 5.0f, 25.0f, &rigidbodies);
            rb->linearVelocity = forward(player->cameraController->camera->transform) * 25.0f;
            collider = (BoxCollider*)newTrashCanEntity->transform.children[0]->entity->components[component::kBoxCollider];
            collider->isActive = false;
            setPosition(&newTrashCanEntity->transform, getPosition(player->cameraController->camera->transform) + forward(player->cameraController->camera->transform));
        }

        if (!input.spawn) {
            canSpawn = true;
        }

        buildImGui(entities, nodeFlags, nodeClicked, player, &sun, &gravity, &enableDirLight);
        updatePlayer(window, &input, player);
        updateRigidBodies(rigidbodies, allColliders, gravity, deltaTime);
        processAnimators(animators, deltaTime, wrenchOffset);
        updateCamera(player);
        setViewProjection(&cameras[0]);
        drawPickingScene(renderers, cameras[0], pickingFBO, pickingShader, &windowData);
        if (isPicking) {
            checkPicker(pickPosition, &windowData, entities, nodeClicked);
        }

        drawScene(renderers, cameras[0], nodeClicked, enableDirLight, &sun, &windowData);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    exitProgram(0);
    return 0;
}

GLFWwindow* createContext(WindowData* windowData) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwSwapInterval(0);
    GLFWwindow* window = glfwCreateWindow(windowData->width, windowData->height, "Pete's Game", NULL, NULL);

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
    glfwSetWindowUserPointer(window, windowData);
    return window;
}

void onScreenChanged(GLFWwindow* window, int width, int height) {
    WindowData* windowData = (WindowData*)glfwGetWindowUserPointer(window);
    glViewport(0, 0, width, height);
    windowData->width = width;
    windowData->height = height;

    for (int i = 0; i < windowData->cameras->size(); i++) {
        windowData->cameras->at(i)->aspectRatio = (float)width / height;
    }
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