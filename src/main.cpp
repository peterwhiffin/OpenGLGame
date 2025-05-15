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
// using std::cout;
// using std::endl;

// All Jolt symbols are in the JPH namespace
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
// Layer that objects can be in, determines which other objects it can collide with
// Typically you at least want to have 1 layer for moving bodies and 1 layer for static bodies, but you can have more
// layers if you want. E.g. you could have a layer for high detail collision (which is not used by the physics simulation
// but only if you do collision testing).
// namespace Layers

/// Class that determines if two object layers can collide
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

// Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to have
// a layer for non-moving and moving objects to avoid having to update a tree full of static objects every frame.
// You can have a 1-on-1 mapping between object layers and broadphase layers (like in this case) but if you have
// many object layers you'll be creating many broad phase trees, which is not efficient. If you want to fine tune
// your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look at the stats reported on the TTY.
// namespace BroadPhaseLayers

// BroadPhaseLayerInterface implementation
// This defines a mapping between object and broadphase layers.
class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface {
   public:
    BPLayerInterfaceImpl() {
        // Create a mapping table from object to broad phase layer
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

// An example contact listener
class MyContactListener : public ContactListener {
   public:
    // See: ContactListener
    virtual ValidateResult OnContactValidate(const Body& inBody1, const Body& inBody2, RVec3Arg inBaseOffset, const CollideShapeResult& inCollisionResult) override {
        // Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
        return ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    virtual void OnContactAdded(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold, ContactSettings& ioSettings) override {
    }

    virtual void OnContactPersisted(const Body& inBody1, const Body& inBody2, const ContactManifold& inManifold, ContactSettings& ioSettings) override {
    }

    virtual void OnContactRemoved(const SubShapeIDPair& inSubShapePair) override {
    }
};

// An example activation listener
class MyBodyActivationListener : public BodyActivationListener {
   public:
    virtual void OnBodyActivated(const BodyID& inBodyID, uint64 inBodyUserData) override {
    }

    virtual void OnBodyDeactivated(const BodyID& inBodyID, uint64 inBodyUserData) override {
    }
};
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
    // ImGui::GetIO().FontGlobalScale = 14.0f;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    // ImVec4* colors = ImGui::GetStyle().Colors;

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
    Scene* scene = new Scene;
    //----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    //
    //----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    // Register allocation hook. In this example we'll just let Jolt use malloc / free but you can override these if you want (see Memory.h).
    // This needs to be done before any other Jolt function is called.
    RegisterDefaultAllocator();

    // Install trace and assert callbacks
    Trace = TraceImpl;
    JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)

    // Create a factory, this class is responsible for creating instances of classes based on their name or hash and is mainly used for deserialization of saved data.
    // It is not directly used in this example but still required.
    Factory::sInstance = new Factory();

    // Register all physics types with the factory and install their collision handlers with the CollisionDispatch class.
    // If you have your own custom shape types you probably need to register their handlers with the CollisionDispatch before calling this function.
    // If you implement your own default material (PhysicsMaterial::sDefault) make sure to initialize it before this function or else this function will create one for you.
    RegisterTypes();

    // We need a temp allocator for temporary allocations during the physics update. We're
    // pre-allocating 10 MB to avoid having to do allocations during the physics update.
    // B.t.w. 10 MB is way too much for this example but it is a typical value you can use.
    // If you don't want to pre-allocate you can also use TempAllocatorMalloc to fall back to
    // malloc / free.
    TempAllocatorImpl temp_allocator(10 * 1024 * 1024);

    // We need a job system that will execute physics jobs on multiple threads. Typically
    // you would implement the JobSystem interface yourself and let Jolt Physics run on top
    // of your own job scheduler. JobSystemThreadPool is an example implementation.
    JobSystemThreadPool job_system(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);

    // This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
    // Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
    const uint cMaxBodies = 65536;

    // This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
    const uint cNumBodyMutexes = 0;

    // This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
    // body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
    // too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
    // Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
    const uint cMaxBodyPairs = 65536;

    // This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
    // number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
    // Note: This value is low because this is a simple test. For a real project use something in the order of 10240.
    const uint cMaxContactConstraints = 10240;

    const int cCollisionSteps = 1;
    // Create mapping table from object layer to broadphase layer
    // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
    // Also have a look at BroadPhaseLayerInterfaceTable or BroadPhaseLayerInterfaceMask for a simpler interface.
    BPLayerInterfaceImpl broad_phase_layer_interface;

    // Create class that filters object vs broadphase layers
    // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
    // Also have a look at ObjectVsBroadPhaseLayerFilterTable or ObjectVsBroadPhaseLayerFilterMask for a simpler interface.
    ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;

    // Create class that filters object vs object layers
    // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
    // Also have a look at ObjectLayerPairFilterTable or ObjectLayerPairFilterMask for a simpler interface.
    ObjectLayerPairFilterImpl object_vs_object_layer_filter;

    // Now we can create the actual physics system.
    PhysicsSystem physics_system;
    physics_system.Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, broad_phase_layer_interface, object_vs_broadphase_layer_filter, object_vs_object_layer_filter);
    physics_system.SetGravity(vec3(0.0f, -18.0f, 0.0f));
    // A body activation listener gets notified when bodies activate and go to sleep
    // Note that this is called from a job so whatever you do here needs to be thread safe.
    // Registering one is entirely optional.
    MyBodyActivationListener body_activation_listener;
    physics_system.SetBodyActivationListener(&body_activation_listener);

    // A contact listener gets notified when bodies (are about to) collide, and when they separate again.
    // Note that this is called from a job so whatever you do here needs to be thread safe.
    // Registering one is entirely optional.
    MyContactListener contact_listener;
    physics_system.SetContactListener(&contact_listener);

    // The main way to interact with the bodies in the physics system is through the body interface. There is a locking and a non-locking
    // variant of this. We're going to use the locking version (even though we're not planning to access bodies from multiple threads)
    // scene->bodyInterface = &physics_system.GetBodyInterfaceNoLock();
    scene->bodyInterface = &physics_system.GetBodyInterface();

    // Next we can create a rigid body to serve as the floor, we make a large box
    // Create the settings for the collision volume (the shape).
    // Note that for simple shapes (like boxes) you can also directly construct a BoxShape.

    // Now create a dynamic body to bounce on the floor
    // Note that this uses the shorthand version of creating and adding a body to the world
    // BodyCreationSettings sphere_settings(new SphereShape(0.5f), RVec3(0.0_r, 2.0_r, 0.0_r), Quat::sIdentity(), EMotionType::Dynamic, Layers::MOVING);
    // BodyID sphere_id = body_interface.CreateAndAddBody(sphere_settings, EActivation::Activate);

    // Now you can interact with the dynamic body, in this case we're going to give it a velocity.
    // (note that if we had used CreateBody then we could have set the velocity straight on the body before adding it to the physics system)
    // body_interface.SetLinearVelocity(sphere_id, Vec3(0.0f, -5.0f, 0.0f));
    /* while (body_interface.IsActive(sphere_id)) {
        // Next step

        // Output current position and velocity of the sphere
        RVec3 position = body_interface.GetCenterOfMassPosition(sphere_id);
        Vec3 velocity = body_interface.GetLinearVelocity(sphere_id);
        cout << "Step " << step << ": Position = (" << position.GetX() << ", " << position.GetY() << ", " << position.GetZ() << "), Velocity = (" << velocity.GetX() << ", " << velocity.GetY() << ", " << velocity.GetZ() << ")" << endl;

        // If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
        const int cCollisionSteps = 1;

        // Step the world
        physics_system.Update(cDeltaTime, cCollisionSteps, &temp_allocator, &job_system);
    } */

    //----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    //
    //----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

    unsigned int pickingShader;
    unsigned int pickingFBO, pickingRBO, pickingTexture;
    unsigned int whiteTexture;
    unsigned int blackTexture;
    unsigned int blueTexture;
    unsigned char whitePixel[4] = {255, 255, 255, 255};
    unsigned char blackPixel[4] = {0, 0, 0, 255};
    unsigned char bluePixel[4] = {0, 0, 255, 255};

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

    if (findLastScene(&scenePath)) {
        loadScene(scene, scenePath);

        // loadDefaultScene(scene);
    } else {
        loadDefaultScene(scene);
    }

    // We simulate the physics world in discrete time steps. 60 Hz is a good rate to update the physics system.

    // Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance (it's pointless here because we only have 2 bodies).
    // You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
    // Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
    // physics_system.OptimizeBroadPhase();

    // Now we're ready to simulate the body, keep simulating until it goes to sleep

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
        scene->physicsAccum += scene->deltaTime;
        updateRigidBodies(scene);

        //-----NEW PHYSICS-----
        /* RVec3 position = body_interface.GetCenterOfMassPosition(sphere_id);
        Vec3 velocity = body_interface.GetLinearVelocity(sphere_id);
        cout << "Step " << step << ": Position = (" << position.GetX() << ", " << position.GetY() << ", " << position.GetZ() << "), Velocity = (" << velocity.GetX() << ", " << velocity.GetY() << ", " << velocity.GetZ() << ")" << endl;
 */
        // If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).

        // Step the world
        if (scene->physicsAccum >= scene->cDeltaTime) {
            physics_system.Update(scene->cDeltaTime, cCollisionSteps, &temp_allocator, &job_system);
            scene->physicsAccum -= scene->cDeltaTime;
            scene->physicsTicked = true;
        } else {
            scene->physicsTicked = false;
        }
        //-----NEW PHYSICS-----

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

    // Remove the sphere from the physics system. Note that the sphere itself keeps all of its state and can be re-added at any time.
    // body_interface.RemoveBody(rb->joltBody);

    // Destroy the sphere. After this the sphere ID is no longer valid.
    // body_interface.DestroyBody(rb->joltBody);

    // Remove and destroy the floor
    // body_interface.RemoveBody(floor->GetID());
    // body_interface.DestroyBody(floor->GetID());

    // Unregisters all types with the factory and cleans up the default material
    // UnregisterTypes();

    // Destroy the factory
    // delete Factory::sInstance;
    // Factory::sInstance = nullptr;
    exitProgram(scene, 0);
    return 0;
}