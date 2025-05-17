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
#include <iostream>
#include <cstdarg>
#include <thread>

using namespace JPH;

// If you want your code to compile using single or double precision write 0.0_r to get a Real value that compiles to double or float depending if JPH_DOUBLE_PRECISION is set or not.

static void TraceImpl(const char* inFMT, ...) {
    // Format the message
    va_list list;
    va_start(list, inFMT);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), inFMT, list);
    va_end(list);

    // Print to the TTY
}

#ifdef JPH_ENABLE_ASSERTS

// Callback for asserts, connect this to your own assert handler if you have one
static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, uint inLine) {
    // Print to the TTY

    // Breakpoint
    return true;
};

#endif  // JPH_ENABLE_ASSERTS

class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter {
   public:
    virtual bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override {
        switch (inObject1) {
            case Layers::NON_MOVING:
                return inObject2 == Layers::MOVING;  // Non moving only collides with moving
            case Layers::MOVING:
                return true;  // Moving collides with everything
            default:
                // JPH_ASSERT(false);
                return false;
        }
    }
};

class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface {
   public:
    BPLayerInterfaceImpl() {
        mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
        mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
    }

    virtual uint GetNumBroadPhaseLayers() const override {
        return BroadPhaseLayers::NUM_LAYERS;
    }

    virtual BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override {
        // JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
        return mObjectToBroadPhase[inLayer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override {
        switch ((BroadPhaseLayer::Type)inLayer) {
            case (BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:
                return "NON_MOVING";
            case (BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:
                return "MOVING";
            default:
                // JPH_ASSERT(false);
                return "INVALID";
        }
    }
#endif  // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

   private:
    BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

/// Class that determines if an object layer can collide with a broadphase layer
class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter {
   public:
    virtual bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override {
        switch (inLayer1) {
            case Layers::NON_MOVING:
                return inLayer2 == BroadPhaseLayers::MOVING;
            case Layers::MOVING:
                return true;
            default:
                // JPH_ASSERT(false);
                return false;
        }
    }
};

void exitProgram(Scene* scene, int code) {
    UnregisterTypes();
    delete Factory::sInstance;
    Factory::sInstance = nullptr;

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
    vec2 v(scene->windowData.viewportWidth / 4.0f, scene->windowData.viewportHeight / 4.0f);
    glUniform2fv(8, 1, &v.x);

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
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.12f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_InputTextCursor] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.35f, 0.58f, 0.86f);
    colors[ImGuiCol_TabSelected] = ImVec4(0.20f, 0.41f, 0.68f, 1.00f);
    colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_TabDimmed] = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
    colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
    colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextLink] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_TreeLines] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavCursor] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
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
        glUniform3fv(glGetUniformLocation(shader, (base + ".position").c_str()), 1, getPosition(scene, pointLight->entityID).mF32);
        glUniform3fv(glGetUniformLocation(shader, (base + ".color").c_str()), 1, pointLight->color.mF32);
        glUniform1f(glGetUniformLocation(shader, (base + ".brightness").c_str()), pointLight->brightness);
    }

    for (int i = 0; i < numSpotLights; i++) {
        SpotLight* spotLight = &scene->spotLights[i];
        std::string base = "spotLights[" + std::to_string(i) + "]";
        glUniform3fv(glGetUniformLocation(shader, (base + ".position").c_str()), 1, getPosition(scene, spotLight->entityID).mF32);
        glUniform3fv(glGetUniformLocation(shader, (base + ".color").c_str()), 1, spotLight->color.mF32);
        glUniform1f(glGetUniformLocation(shader, (base + ".brightness").c_str()), spotLight->brightness);
        glUniform1f(glGetUniformLocation(shader, (base + ".cutOff").c_str()), glm::cos(glm::radians(spotLight->cutoff)));
        glUniform1f(glGetUniformLocation(shader, (base + ".outerCutOff").c_str()), glm::cos(glm::radians(spotLight->outerCutoff)));
        // glUniform1i(glGetUniformLocation(shader, (base + ".shadowMap").c_str()), uniform_location::kTextureShadowMapUnit + i);
    }

    glUseProgram(scene->ssaoShader);
    vec2 v(scene->windowData.viewportWidth / 4.0f, scene->windowData.viewportHeight / 4.0f);
    glUniform2fv(8, 1, &v.x);
    scene->AORadius = 0.06f;
    scene->AOBias = 0.04f;
    scene->AOPower = 2.02f;
}

void loadDefaultScene(Scene* scene) {
    for (int i = 0; i < 2; i++) {
        Entity* pointLightEntity = getNewEntity(scene, "PointLight");
        PointLight* pointLight = addPointLight(scene, pointLightEntity->entityID);
        setPosition(scene, pointLightEntity->entityID, vec3(2.0f + i / 2, 3.0f, 1.0f + i / 2));
        pointLight->color = vec3(1.0f, 1.0f, 1.0f);
        pointLight->isActive = true;
        pointLight->brightness = 1.0f;
    }

    uint32_t spotLightEntityID = getNewEntity(scene, "SpotLight")->entityID;
    SpotLight* spotLight = addSpotLight(scene, spotLightEntityID);
    spotLight->isActive = true;
    spotLight->color = vec3(1.0f, 1.0f, 1.0f);
    spotLight->brightness = 6.0f;
    spotLight->cutoff = 15.5f;
    spotLight->outerCutoff = 55.5f;
    spotLight->shadowWidth = 800;
    spotLight->shadowHeight = 600;

    uint32_t levelEntity = createEntityFromModel(scene, scene->testRoom->rootNode, INVALID_ID, true, INVALID_ID, true, false);
    uint32_t armsID = createEntityFromModel(scene, scene->wrenchArms->rootNode, INVALID_ID, false, INVALID_ID, true, false);
    Transform* armsTransform = getTransform(scene, armsID);
    addAnimator(scene, armsID, scene->wrenchArms);

    Entity* wrenchParent = getNewEntity(scene, "WrenchParent");
    Player* player = createPlayer(scene);
    player->armsID = armsID;
    setParent(scene, armsID, wrenchParent->entityID);
    setParent(scene, wrenchParent->entityID, player->cameraController->cameraTargetEntityID);
    setParent(scene, spotLightEntityID, player->cameraController->cameraEntityID);
    setLocalRotation(scene, wrenchParent->entityID, quat::sEulerAngles(vec3(0.0f, 0.0f, 0.0f)));
    setLocalPosition(scene, wrenchParent->entityID, scene->wrenchOffset);
    setLocalRotation(scene, spotLightEntityID, quat::sEulerAngles(vec3(0.0f, 0.0f, 0.0f)));
    setLocalPosition(scene, spotLightEntityID, vec3(0.0f, 0.0f, 1.0f));
}

int main() {
    const uint cMaxBodies = 65536;
    const uint cNumBodyMutexes = 0;
    const uint cMaxBodyPairs = 65536;
    const uint cMaxContactConstraints = 10240;
    const uint cCollisionSteps = 1;

    unsigned int pickingShader;
    unsigned int pickingFBO, pickingRBO, pickingTexture;
    unsigned int whiteTexture;
    unsigned int blackTexture;
    unsigned int blueTexture;
    unsigned char whitePixel[4] = {255, 255, 255, 255};
    unsigned char blackPixel[4] = {0, 0, 0, 255};
    unsigned char bluePixel[4] = {0, 0, 255, 255};

    Scene* scene = new Scene;
    std::string scenePath;
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
    scene->debugShader = loadShader("../src/shaders/debugShader.vs", "../src/shaders/debugShader.fs");

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
    defaultMaterial->baseColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    defaultMaterial->shader = scene->lightingShader;
    defaultMaterial->name = "default";
    scene->materialMap[defaultMaterial->name] = defaultMaterial;

    scene->testRoom = loadModel(scene, "../resources/models/testroom/testroom.gltf", &scene->textures, scene->lightingShader, true);
    scene->trashcanModel = loadModel(scene, "../resources/models/trashcan/trashcan.gltf", &scene->textures, scene->lightingShader, true);
    scene->wrenchArms = loadModel(scene, "../resources/models/Arms/wrencharms.gltf", &scene->textures, scene->lightingShader, true);

    RegisterDefaultAllocator();
    Trace = TraceImpl;
    JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)
    Factory::sInstance = new Factory();
    RegisterTypes();
    TempAllocatorImpl temp_allocator(10 * 1024 * 1024);
    JobSystemThreadPool job_system(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);
    BPLayerInterfaceImpl broad_phase_layer_interface;
    ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;
    ObjectLayerPairFilterImpl object_vs_object_layer_filter;
    PhysicsSystem physics_system;
    physics_system.Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, broad_phase_layer_interface, object_vs_broadphase_layer_filter, object_vs_object_layer_filter);
    physics_system.SetGravity(vec3(0.0f, -18.0f, 0.0f));
    scene->bodyInterface = &physics_system.GetBodyInterface();
    scene->physicsSystem = &physics_system;

    if (findLastScene(&scenePath)) {
        loadScene(scene, scenePath);
    } else {
        loadDefaultScene(scene);
    }

    for (MeshRenderer& renderer : scene->meshRenderers) {
        mapBones(scene, &renderer);
    }

    scene->debugRenderer = new MyDebugRenderer();
    JPH::DebugRenderer::sInstance = scene->debugRenderer;
    JPH::BodyManager::DrawSettings drawSettings;
    drawSettings.mDrawShape = true;
    drawSettings.mDrawShapeWireframe = true;
    drawSettings.mDrawShapeColor = JPH::BodyManager::EShapeColor::MotionTypeColor;

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
    glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(mat4), NULL, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, scene->matricesUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, scene->matricesUBO);

    setFlags();
    initializeIMGUI(window);
    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;

    scene->currentFrame = static_cast<float>(glfwGetTime());
    scene->lastFrame = scene->currentFrame;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        updateTime(scene);
        updateInput(window, &scene->input);
        updatePlayer(scene, window, &scene->input, scene->player);
        scene->physicsAccum += scene->deltaTime;
        updateRigidBodies(scene);

        if (scene->physicsAccum >= scene->cDeltaTime) {
            physics_system.Update(scene->cDeltaTime, cCollisionSteps, &temp_allocator, &job_system);
            scene->physicsAccum -= scene->cDeltaTime;
            scene->physicsTicked = true;
        } else {
            scene->physicsTicked = false;
        }

        updateAnimators(scene);
        updateCamera(scene);

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