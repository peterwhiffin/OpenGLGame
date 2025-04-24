#include "loader.h"
#include "component.h"
#include "input.h"
#include "camera.h"
#include "physics.h"
#include "animation.h"
#include "player.h"
#include "renderer.h"
#include "transform.h"
#include "shader.h"
#include "debug.h"

void exitProgram(int code) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    exit(code);
}

void onScreenChanged(GLFWwindow* window, int width, int height) {
    Scene* scene = (Scene*)glfwGetWindowUserPointer(window);
    glViewport(0, 0, width, height);
    scene->windowData.width = width;
    scene->windowData.height = height;

    for (int i = 0; i < scene->cameras.size(); i++) {
        scene->cameras[i]->aspectRatio = (float)width / height;
    }
}

GLFWwindow* createContext(Scene* scene) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwSwapInterval(0);
    GLFWwindow* window = glfwCreateWindow(scene->windowData.width, scene->windowData.height, "Pete's Game", NULL, NULL);

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
    glfwSetWindowUserPointer(window, scene);
    return window;
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

void updateTime(Scene* scene) {
    scene->currentFrame = static_cast<float>(glfwGetTime());
    scene->deltaTime = scene->currentFrame - scene->lastFrame;
    scene->lastFrame = scene->currentFrame;
}

int main() {
    unsigned int pickingShader;
    unsigned int pickingFBO, pickingRBO, pickingTexture;
    unsigned int whiteTexture;
    unsigned int blackTexture;
    unsigned int nodeclicked = INVALID_ID;
    unsigned char whitePixel[4] = {255, 255, 255, 255};
    unsigned char blackPixel[4] = {0, 0, 0, 255};

    Scene* scene = new Scene();
    scene->windowData.width = 800;
    scene->windowData.height = 600;

    GLFWwindow* window = createContext(scene);

    glGenTextures(1, &whiteTexture);
    glGenTextures(1, &blackTexture);
    glBindTexture(GL_TEXTURE_2D, whiteTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
    glBindTexture(GL_TEXTURE_2D, blackTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, blackPixel);

    Texture white;
    Texture black;
    white.id = whiteTexture;
    black.id = blackTexture;
    white.path = "white";
    black.path = "black";

    scene->textures.push_back(white);
    scene->textures.push_back(black);
    scene->gravity = -18.81f;
    scene->sun.position = glm::vec3(-3.0f, 30.0f, -2.0f);
    scene->sun.lookDirection = glm::vec3(0.0f, 0.0f, 0.0f);
    scene->sun.color = glm::vec3(1.0f, 1.0f, 1.0f);
    scene->sun.ambient = glm::vec3(0.21f);
    scene->sun.diffuse = glm::vec3(0.94f);
    scene->sun.specular = glm::vec3(0.18f);
    scene->sun.ambientBrightness = 1.7f;
    scene->sun.diffuseBrightness = 2.0f;
    scene->sun.isEnabled = true;

    createPickingFBO(scene, &pickingFBO, &pickingRBO, &pickingTexture);
    createGBuffer(scene);
    createFullScreenQuad(scene);

    unsigned int defaultShader = loadShader("../src/shaders/litshader.vs", "../src/shaders/litshader.fs");
    scene->fullscreenShader = loadShader("../src/shaders/fullscreenshader.vs", "../src/shaders/fullscreenshader.fs");
    scene->gBufferShader = loadShader("../src/shaders/gbuffershader.vs", "../src/shaders/gbuffershader.fs");
    pickingShader = loadShader("../src/shaders/pickingshader.vs", "../src/shaders/pickingshader.fs");
    scene->defaultShader = defaultShader;

    glUseProgram(scene->gBufferShader);
    glUniform1i(uniform_location::kTextureDiffuse, uniform_location::kTextureDiffuseUnit);
    glUniform1i(uniform_location::kTextureSpecular, uniform_location::kTextureSpecularUnit);

    glUseProgram(scene->fullscreenShader);
    glUniform1i(glGetUniformLocation(scene->fullscreenShader, "gPosition"), 0);
    glUniform1i(glGetUniformLocation(scene->fullscreenShader, "gNormal"), 1);
    glUniform1i(glGetUniformLocation(scene->fullscreenShader, "gAlbedoSpec"), 2);
    glUniform3fv(glGetUniformLocation(scene->fullscreenShader, "dirLight.position"), 1, glm::value_ptr(scene->sun.position));
    glUniform3fv(glGetUniformLocation(scene->fullscreenShader, "dirLight.ambient"), 1, glm::value_ptr(scene->sun.ambient));
    glUniform3fv(glGetUniformLocation(scene->fullscreenShader, "dirLight.diffuse"), 1, glm::value_ptr(scene->sun.diffuse));
    glUniform3fv(glGetUniformLocation(scene->fullscreenShader, "dirLight.specular"), 1, glm::value_ptr(scene->sun.specular));

    glUseProgram(defaultShader);
    glUniform1i(uniform_location::kTextureDiffuse, uniform_location::kTextureDiffuseUnit);
    glUniform1i(uniform_location::kTextureSpecular, uniform_location::kTextureSpecularUnit);
    glUniform1i(uniform_location::kTextureShadowMap, uniform_location::kTextureShadowMapUnit);
    glUniform1i(uniform_location::kTextureNoise, uniform_location::kTextureNoiseUnit);
    glUniform3fv(glGetUniformLocation(defaultShader, "dirLight.position"), 1, glm::value_ptr(scene->sun.position));
    glUniform3fv(glGetUniformLocation(defaultShader, "dirLight.ambient"), 1, glm::value_ptr(scene->sun.ambient));
    glUniform3fv(glGetUniformLocation(defaultShader, "dirLight.diffuse"), 1, glm::value_ptr(scene->sun.diffuse));
    glUniform3fv(glGetUniformLocation(defaultShader, "dirLight.specular"), 1, glm::value_ptr(scene->sun.specular));

    Entity* pointLightEntity = getNewEntity(scene, "PointLight");
    PointLight* pointLight = addPointLight(scene, pointLightEntity->id);
    setPosition(scene, pointLightEntity->id, glm::vec3(2.0f, 3.0f, 1.0f));
    pointLight->ambient = glm::vec3(0.05f);
    pointLight->diffuse = glm::vec3(0.8f);
    pointLight->specular = glm::vec3(1.0f);
    pointLight->constant = 1.0f;
    pointLight->linear = 0.09f;
    pointLight->quadratic = 0.032f;
    pointLight->isActive = 1;
    pointLight->brightness = 2.0f;

    glUniform3fv(glGetUniformLocation(defaultShader, "pointLights[0].position"), 1, glm::value_ptr(getPosition(scene, pointLightEntity->id)));
    glUniform3fv(glGetUniformLocation(defaultShader, "pointLights[0].ambient"), 1, glm::value_ptr(pointLight->ambient));
    glUniform3fv(glGetUniformLocation(defaultShader, "pointLights[0].diffuse"), 1, glm::value_ptr(pointLight->diffuse));
    glUniform3fv(glGetUniformLocation(defaultShader, "pointLights[0].specular"), 1, glm::value_ptr(pointLight->specular));
    glUniform1f(glGetUniformLocation(defaultShader, "pointLights[0].constant"), pointLight->constant);
    glUniform1f(glGetUniformLocation(defaultShader, "pointLights[0].linear"), pointLight->linear);
    glUniform1f(glGetUniformLocation(defaultShader, "pointLights[0].quadratic"), pointLight->quadratic);
    glUniform1f(glGetUniformLocation(defaultShader, "pointLights[0].brightness"), pointLight->brightness);
    glUniform1ui(glGetUniformLocation(defaultShader, "pointLights[0].isActive"), pointLight->isActive);

    glUseProgram(scene->fullscreenShader);
    glUniform3fv(glGetUniformLocation(scene->fullscreenShader, "pointLights[0].position"), 1, glm::value_ptr(getPosition(scene, pointLightEntity->id)));
    glUniform3fv(glGetUniformLocation(scene->fullscreenShader, "pointLights[0].ambient"), 1, glm::value_ptr(pointLight->ambient));
    glUniform3fv(glGetUniformLocation(scene->fullscreenShader, "pointLights[0].diffuse"), 1, glm::value_ptr(pointLight->diffuse));
    glUniform3fv(glGetUniformLocation(scene->fullscreenShader, "pointLights[0].specular"), 1, glm::value_ptr(pointLight->specular));
    glUniform1f(glGetUniformLocation(scene->fullscreenShader, "pointLights[0].constant"), pointLight->constant);
    glUniform1f(glGetUniformLocation(scene->fullscreenShader, "pointLights[0].linear"), pointLight->linear);
    glUniform1f(glGetUniformLocation(scene->fullscreenShader, "pointLights[0].quadratic"), pointLight->quadratic);
    glUniform1f(glGetUniformLocation(scene->fullscreenShader, "pointLights[0].brightness"), pointLight->brightness);
    glUniform1ui(glGetUniformLocation(scene->fullscreenShader, "pointLights[0].isActive"), pointLight->isActive);

    Model* testRoom = loadModel("../resources/models/testroom/testroom.gltf", &scene->textures, defaultShader);
    Model* wrench = loadModel("../resources/models/wrench/wrench.gltf", &scene->textures, defaultShader);
    scene->trashcanModel = loadModel("../resources/models/trashcan/trashcan.gltf", &scene->textures, defaultShader);

    uint32_t levelEntity = createEntityFromModel(scene, testRoom->rootNode, INVALID_ID, true);
    uint32_t trashCanEntity = createEntityFromModel(scene, scene->trashcanModel->rootNode, INVALID_ID, true);
    uint32_t wrenchEntity = createEntityFromModel(scene, wrench->rootNode, INVALID_ID, false);

    addAnimator(scene, wrenchEntity, wrench);
    setPosition(scene, trashCanEntity, glm::vec3(1.0f, 3.0f, 2.0f));
    getBoxCollider(scene, trashCanEntity)->isActive = false;
    RigidBody* rb = addRigidbody(scene, trashCanEntity);
    rb->mass = 10.0f;
    rb->linearDrag = 3.0f;
    rb->friction = 5.0f;

    Player* player = createPlayer(scene);
    setParent(scene, wrenchEntity, player->cameraController->cameraTargetEntityID);
    setLocalRotation(scene, wrenchEntity, glm::quat(glm::vec3(glm::radians(0.0f), glm::radians(180.0f), 0.0f)));
    setLocalPosition(scene, wrenchEntity, scene->wrenchOffset);

    setFlags();
    initializeIMGUI(window);
    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    InputActions input = InputActions();
    scene->currentFrame = static_cast<float>(glfwGetTime());
    scene->lastFrame = scene->currentFrame;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        updateTime(scene);
        updateInput(window, &input);
        updatePlayer(scene, window, &input, player);
        updateRigidBodies(scene);
        updateAnimators(scene);
        updateCamera(scene, player);
        drawGBuffer(scene);
        drawFullScreenQuad(scene);
        // drawScene(scene, nodeclicked);
        drawDebug(scene, nodeFlags, nodeclicked, player);
        glfwSwapBuffers(window);
    }

    exitProgram(0);
    return 0;
}