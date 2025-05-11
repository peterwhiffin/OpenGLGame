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
#include "sceneloader.h"
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

JPH_SUPPRESS_WARNINGS

// All Jolt symbols are in the JPH namespace
using namespace JPH;

// If you want your code to compile using single or double precision write 0.0_r to get a Real value that compiles to double or float depending if JPH_DOUBLE_PRECISION is set or not.
using namespace JPH::literals;
namespace Layers {
static constexpr ObjectLayer NON_MOVING = 0;
static constexpr ObjectLayer MOVING = 1;
static constexpr ObjectLayer NUM_LAYERS = 2;
};  // namespace Layers
void exitProgram(Scene* scene, int code) {
    deleteBuffers(scene);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    exit(code);
}

void onScreenChanged(GLFWwindow* window, int width, int height) {
    Scene* scene = (Scene*)glfwGetWindowUserPointer(window);
    /* scene->windowData.viewportWidth = width;
    scene->windowData.viewportHeight = height; */

    glViewport(0, 0, width, height);
    /* scene->windowData.width = width;
    scene->windowData.height = height; */

    glUseProgram(scene->ssaoShader);
    glUniform2fv(8, 1, glm::value_ptr(glm::vec2(scene->windowData.viewportWidth / 4.0f, scene->windowData.viewportHeight / 4.0f)));

    for (int i = 0; i < scene->cameras.size(); i++) {
        scene->cameras[i]->aspectRatio = (float)scene->windowData.viewportWidth / scene->windowData.viewportHeight;
    }

    resizeBuffers(scene);
}

GLFWwindow* createContext(Scene* scene) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwSwapInterval(1);
    GLFWwindow* window = glfwCreateWindow(scene->windowData.width, scene->windowData.height, "Pete's Game", NULL, NULL);

    if (window == NULL) {
        std::cout << "Failed to create window" << std::endl;
        exitProgram(scene, -1);
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to load GLAD" << std::endl;
        exitProgram(scene, -1);
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
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    float aspectRatio = 1920.0f / 1080.0f;
    io.Fonts->AddFontFromFileTTF("../resources/fonts/Karla-Regular.ttf", aspectRatio * 8);
    // ImGui::GetIO().FontGlobalScale = 14.0f;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    // ImVec4* colors = ImGui::GetStyle().Colors;
}

void updateTime(Scene* scene) {
    scene->currentFrame = static_cast<float>(glfwGetTime());
    scene->deltaTime = scene->currentFrame - scene->lastFrame;
    scene->lastFrame = scene->currentFrame;
}

void initializeLights(Scene* scene, unsigned int shader) {
    glUseProgram(shader);
    int numPointLights = scene->pointLights.size();
    int numSpotLights = scene->spotLights.size();
    glUniform1i(6, numSpotLights);
    glUniform1i(7, numPointLights);

    for (int i = 0; i < numPointLights; i++) {
        PointLight* pointLight = &scene->pointLights[i];
        std::string base = "pointLights[" + std::to_string(i) + "]";
        glUniform3fv(glGetUniformLocation(shader, (base + ".position").c_str()), 1, glm::value_ptr(getPosition(scene, pointLight->entityID)));
        glUniform3fv(glGetUniformLocation(shader, (base + ".color").c_str()), 1, glm::value_ptr(pointLight->color));
        glUniform1f(glGetUniformLocation(shader, (base + ".brightness").c_str()), pointLight->brightness);
    }

    for (int i = 0; i < numSpotLights; i++) {
        SpotLight* spotLight = &scene->spotLights[i];
        std::string base = "spotLights[" + std::to_string(i) + "]";
        glUniform3fv(glGetUniformLocation(shader, (base + ".position").c_str()), 1, glm::value_ptr(getPosition(scene, spotLight->entityID)));
        glUniform3fv(glGetUniformLocation(shader, (base + ".color").c_str()), 1, glm::value_ptr(spotLight->color));
        glUniform1f(glGetUniformLocation(shader, (base + ".brightness").c_str()), spotLight->brightness);
        glUniform1f(glGetUniformLocation(shader, (base + ".cutOff").c_str()), glm::cos(glm::radians(spotLight->cutoff)));
        glUniform1f(glGetUniformLocation(shader, (base + ".outerCutOff").c_str()), glm::cos(glm::radians(spotLight->outerCutoff)));
        // glUniform1i(glGetUniformLocation(shader, (base + ".shadowMap").c_str()), uniform_location::kTextureShadowMapUnit + i);
    }

    glUseProgram(scene->ssaoShader);
    glUniform2fv(8, 1, glm::value_ptr(glm::vec2(scene->windowData.viewportWidth / 4.0f, scene->windowData.viewportHeight / 4.0f)));
    scene->AORadius = 0.06f;
    scene->AOBias = 0.04f;
    scene->AOPower = 2.02f;
}

void loadDefaultScene(Scene* scene) {
    for (int i = 0; i < 2; i++) {
        Entity* pointLightEntity = getNewEntity(scene, "PointLight");
        PointLight* pointLight = addPointLight(scene, pointLightEntity->entityID);
        setPosition(scene, pointLightEntity->entityID, glm::vec3(2.0f + i / 2, 3.0f, 1.0f + i / 2));
        pointLight->color = glm::vec3(1.0f);
        pointLight->isActive = true;
        pointLight->brightness = 1.0f;
    }

    Entity* spotLightEntity = getNewEntity(scene, "SpotLight");
    SpotLight* spotLight = addSpotLight(scene, spotLightEntity->entityID);
    spotLight->isActive = true;
    spotLight->color = glm::vec3(1.0f);
    spotLight->brightness = 6.0f;
    spotLight->cutoff = 15.5f;
    spotLight->outerCutoff = 55.5f;
    spotLight->shadowWidth = 800;
    spotLight->shadowHeight = 600;

    // uint32_t wrenchEntity = createEntityFromModel(scene, scene->wrench->rootNode, INVALID_ID, false);
    uint32_t levelEntity = createEntityFromModel(scene, scene->testRoom->rootNode, INVALID_ID, true, INVALID_ID, true);
    uint32_t trashCanEntity = createEntityFromModel(scene, scene->trashcanModel->rootNode, INVALID_ID, true, INVALID_ID, true);

    uint32_t armsID = createEntityFromModel(scene, scene->wrenchArms->rootNode, INVALID_ID, false, INVALID_ID, true);
    Transform* armsTransform = getTransform(scene, armsID);
    addAnimator(scene, armsID, scene->wrenchArms);
    // addAnimator(scene, wrenchEntity, scene->wrench);
    setPosition(scene, trashCanEntity, glm::vec3(1.0f, 3.0f, 2.0f));
    getBoxCollider(scene, trashCanEntity)->isActive = false;

    Entity* tcanEnt = getEntity(scene, trashCanEntity);
    Transform* tcantrans = getTransform(scene, trashCanEntity);
    tcanEnt->name = "trashcanBase";
    Entity* lident = getEntity(scene, tcantrans->childEntityIds[0]);
    lident->name = "trashcanLid";

    RigidBody* rb = addRigidbody(scene, trashCanEntity);
    rb->mass = 10.0f;
    rb->linearDrag = 3.0f;
    rb->friction = 10.0f;

    Entity* wrenchParent = getNewEntity(scene, "WrenchParent");

    Player* player = createPlayer(scene);
    player->armsID = armsID;
    setParent(scene, armsID, wrenchParent->entityID);
    setParent(scene, wrenchParent->entityID, player->cameraController->cameraTargetEntityID);
    setParent(scene, spotLightEntity->entityID, player->cameraController->cameraEntityID);
    setLocalRotation(scene, wrenchParent->entityID, glm::quat(glm::vec3(glm::radians(0.0f), glm::radians(180.0f), 0.0f)));
    setLocalPosition(scene, wrenchParent->entityID, scene->wrenchOffset);

    setLocalRotation(scene, spotLightEntity->entityID, glm::quat(glm::vec3(glm::radians(0.0f), glm::radians(0.0f), 0.0f)));
    setLocalPosition(scene, spotLightEntity->entityID, glm::vec3(0.0f, 0.0f, 1.0f));
}

int main() {
    unsigned int pickingShader;
    unsigned int pickingFBO, pickingRBO, pickingTexture;
    unsigned int whiteTexture;
    unsigned int blackTexture;
    unsigned int blueTexture;
    unsigned char whitePixel[4] = {255, 255, 255, 255};
    unsigned char blackPixel[4] = {0, 0, 0, 255};
    unsigned char bluePixel[4] = {0, 0, 255, 255};

    std::string scenePath;
    Scene* scene = new Scene;
    scene->windowData.width = 1920;
    scene->windowData.height = 1080;
    scene->windowData.viewportWidth = 1920;
    scene->windowData.viewportHeight = 1080;
    GLFWwindow* window = createContext(scene);

    scene->pickingShader = loadShader("../src/shaders/pickingshader.vs", "../src/shaders/pickingshader.fs");
    scene->depthShader = loadShader("../src/shaders/depthprepassshader.vs", "../src/shaders/depthprepassshader.fs");
    scene->lightingShader = loadShader("../src/shaders/pbrlitshader.vs", "../src/shaders/pbrlitshader.fs");
    scene->ssaoShader = loadShader("../src/shaders/SSAOshader.vs", "../src/shaders/SSAOshader.fs");
    scene->shadowBlurShader = loadShader("../src/shaders/SSAOshader.vs", "../src/shaders/SSAOblurshader.fs");
    scene->blurShader = loadShader("../src/shaders/gaussianblurshader.vs", "../src/shaders/gaussianblurshader.fs");
    scene->postProcessShader = loadShader("../src/shaders/postprocessshader.vs", "../src/shaders/postprocessshader.fs");

    glGenTextures(1, &whiteTexture);
    glGenTextures(1, &blackTexture);
    glGenTextures(1, &blueTexture);
    glBindTexture(GL_TEXTURE_2D, whiteTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
    glBindTexture(GL_TEXTURE_2D, blackTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, blackPixel);
    glBindTexture(GL_TEXTURE_2D, blueTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, bluePixel);

    Texture white;
    Texture black;
    Texture blue;

    white.id = whiteTexture;
    black.id = blackTexture;
    blue.id = blueTexture;
    white.path = "white";
    white.name = "white";
    black.path = "black";
    black.name = "black";
    blue.path = "blue";
    blue.name = "blue";

    scene->textures.push_back(black);
    scene->textures.push_back(white);
    scene->textures.push_back(blue);
    scene->textureMap[white.name] = white;
    scene->textureMap[black.name] = black;
    scene->textureMap[blue.name] = blue;

    Material* defaultMaterial = new Material();
    defaultMaterial->textures.push_back(white);
    defaultMaterial->textures.push_back(white);
    defaultMaterial->textures.push_back(black);
    defaultMaterial->textures.push_back(white);
    defaultMaterial->textures.push_back(blue);
    defaultMaterial->baseColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    defaultMaterial->shader = scene->lightingShader;
    defaultMaterial->name = "default";
    scene->materialMap["default"] = defaultMaterial;

    scene->testRoom = loadModel(scene, "../resources/models/testroom/testroom.gltf", &scene->textures, scene->lightingShader, true);
    scene->wrench = loadModel(scene, "../resources/models/wrench/wrench.gltf", &scene->textures, scene->lightingShader, true);
    scene->trashcanModel = loadModel(scene, "../resources/models/trashcan/trashcan.gltf", &scene->textures, scene->lightingShader, true);
    scene->arms = loadModel(scene, "../resources/models/Arms/DidIdoit.gltf", &scene->textures, scene->lightingShader, true);
    scene->wrenchArms = loadModel(scene, "../resources/models/Arms/wrencharms.gltf", &scene->textures, scene->lightingShader, true);

    if (findLastScene(&scenePath)) {
        loadScene(scene, scenePath);
    } else {
        loadDefaultScene(scene);
    }

    // uint32_t armsID = createEntityFromModel(scene, scene->arms->rootNode, INVALID_ID, false);
    // Transform* armsTransform = getTransform(scene, armsID);
    // addAnimator(scene, armsID, scene->arms);

    /* MeshRenderer* renderer;

    for (int i = 0; i < armsTransform->childEntityIds.size(); i++) {
        renderer = getMeshRenderer(scene, armsTransform->childEntityIds[i]);
        if (renderer != nullptr) {
            break;
        }
    } */

    // mapBones(scene, renderer);

    for (MeshRenderer& renderer : scene->meshRenderers) {
        mapBones(scene, &renderer);
    }

    /* for (int i = 0; i < 12; i++) {
    Entity* spotLightEntity = getNewEntity(scene, "SpotLight");
    SpotLight* spotLight = addSpotLight(scene, spotLightEntity->entityID);
    spotLight->isActive = true;
    spotLight->color = glm::vec3(1.0f);
    spotLight->brightness = 6.0f;
    spotLight->cutoff = 15.5f;
    spotLight->outerCutoff = 55.5f;
    spotLight->shadowWidth = 1024;
    spotLight->shadowHeight = 1024;
}
*/
    /* for (int i = 0; i < 1; i++) {
        Entity* pointLightEntity = getNewEntity(scene, "PointLight");
        PointLight* spotLight = addPointLight(scene, pointLightEntity->entityID);
        spotLight->isActive = true;
        spotLight->color = glm::vec3(1.0f);
        spotLight->brightness = 4.0f;
    } */

    // createPickingFBO(scene);
    createSSAOBuffer(scene);
    createShadowMapDepthBuffers(scene);
    createForwardBuffer(scene);
    createBlurBuffers(scene);
    createFullScreenQuad(scene);
    createEditorBuffer(scene);
    generateSSAOKernel(scene);

    initializeLights(scene, scene->lightingShader);

    glGenBuffers(1, &scene->matricesUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, scene->matricesUBO);
    glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, scene->matricesUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, scene->matricesUBO);

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
        updatePlayer(scene, window, &input, scene->player);
        updateRigidBodies(scene);
        updateAnimators(scene);
        updateCamera(scene);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(GlobalUBO), &scene->matricesUBOData);

        // drawPickingScene(scene);
        // checkPicker(scene, input.cursorPosition);
        drawShadowMaps(scene);
        drawScene(scene);
        drawSSAO(scene);
        drawBlurPass(scene);
        drawFullScreenQuad(scene);

        drawDebug(scene, nodeFlags, scene->player);

        glfwSwapBuffers(window);
    }

    exitProgram(scene, 0);
    return 0;
}