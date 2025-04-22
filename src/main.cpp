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

GLFWwindow* createContext(Scene* scene);
void onScreenChanged(GLFWwindow* window, int width, int height);
void initializeIMGUI(GLFWwindow* window);
void exitProgram(int code);

int main() {
    Scene* scene = new Scene();
    scene->windowData.width = 800;
    scene->windowData.height = 600;
    scene->gravity = -18.81f;

    uint32_t nodeClicked = INVALID_ID;
    Player* player;

    glm::dvec2 pickPosition = glm::dvec2(0, 0);
    glm::vec3 wrenchOffset = glm::vec3(0.3f, -0.3f, -0.5f);

    float currentFrame = 0.0f;
    float lastFrame = 0.0f;
    float dirLightBrightness = 2.1f;
    float ambientBrightness = 1.21f;

    unsigned int nextEntityID = 1;
    unsigned int pickingShader;

    bool enableDirLight = true;
    bool isPicking = false;
    bool canSpawn = true;

    GLFWwindow* window = createContext(scene);
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

    scene->sun.position = glm::vec3(-3.0f, 30.0f, -2.0f);
    scene->sun.lookDirection = glm::vec3(0.0f, 0.0f, 0.0f);
    scene->sun.color = glm::vec3(1.0f, 1.0f, 1.0f);
    scene->sun.ambient = glm::vec3(0.21f);
    scene->sun.diffuse = glm::vec3(0.94f);
    scene->sun.specular = glm::vec3(0.18f);
    scene->sun.ambientBrightness = 1.7f;
    scene->sun.diffuseBrightness = 2.0f;
    scene->sun.isEnabled = true;

    glUniform3fv(glGetUniformLocation(defaultShader, "dirLight.position"), 1, glm::value_ptr(scene->sun.position));
    glUniform3fv(glGetUniformLocation(defaultShader, "dirLight.ambient"), 1, glm::value_ptr(scene->sun.ambient));
    glUniform3fv(glGetUniformLocation(defaultShader, "dirLight.diffuse"), 1, glm::value_ptr(scene->sun.diffuse));
    glUniform3fv(glGetUniformLocation(defaultShader, "dirLight.specular"), 1, glm::value_ptr(scene->sun.specular));

    Texture white;
    Texture black;
    white.id = whiteTexture;
    black.id = blackTexture;
    white.path = "white";
    black.path = "black";
    scene->textures.push_back(white);
    scene->textures.push_back(black);

    Model* testRoom = loadModel("../resources/models/testroom/testroom.gltf", &scene->textures, defaultShader);
    Model* wrench = loadModel("../resources/models/wrench/wrench.gltf", &scene->textures, defaultShader);
    Model* trashcan = loadModel("../resources/models/trashcan/trashcan.gltf", &scene->textures, defaultShader);

    uint32_t trashCanEntity = createEntityFromModel(scene, trashcan, trashcan->rootNode, INVALID_ID, true);
    uint32_t levelEntity = createEntityFromModel(scene, testRoom, testRoom->rootNode, INVALID_ID, true);
    uint32_t wrenchEntity = createEntityFromModel(scene, wrench, wrench->rootNode, INVALID_ID, false);

    addAnimator(scene, wrenchEntity, wrench);

    RigidBody* rb = addRigidbody(scene, trashCanEntity);
    rb->mass = 10.0f;
    rb->linearDrag = 3.0f;
    rb->friction = 5.0f;

    setPosition(scene, trashCanEntity, glm::vec3(1.0f, 3.0f, 2.0f));
    size_t index = scene->colliderIndices[trashCanEntity];
    BoxCollider* collider = &scene->boxColliders[index];
    collider->isActive = false;

    player = createPlayer(scene);
    setParent(scene, wrenchEntity, player->cameraController->cameraTargetEntityID);
    setLocalRotation(scene, wrenchEntity, glm::quat(glm::vec3(glm::radians(0.0f), glm::radians(180.0f), 0.0f)));
    setLocalPosition(scene, wrenchEntity, wrenchOffset);
    unsigned int pickingFBO, pickingRBO, pickingTexture;
    createPickingFBO(scene, &pickingFBO, &pickingRBO, &pickingTexture);
    setFlags();
    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow |
                                   ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                   ImGuiTreeNodeFlags_SpanAvailWidth;

    unsigned char pixel[3];
    currentFrame = static_cast<float>(glfwGetTime());
    lastFrame = currentFrame;

    while (!glfwWindowShouldClose(window)) {
        currentFrame = static_cast<float>(glfwGetTime());
        scene->deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        updateInput(window, &input);

        /* if (input.spawn && canSpawn) {
            canSpawn = false;
            Entity* newTrashCanEntity = createEntityFromModel(&entities, trashcan, trashcan->rootNode, &renderers, nullptr, &allColliders, true, true, &nextEntityID);
            RigidBody* rb = addRigidBody(newTrashCanEntity, 10.0f, 5.0f, 25.0f, &rigidbodies);
            rb->linearVelocity = forward(player->cameraController->camera->transform) * 25.0f;
            collider = (BoxCollider*)newTrashCanEntity->transform.children[0]->entity->components[component::kBoxCollider];
            collider->isActive = false;
            setPosition(&newTrashCanEntity->transform, getPosition(player->cameraController->camera->transform) + forward(player->cameraController->camera->transform));
        } */

        if (!input.spawn) {
            canSpawn = true;
        }

        // buildImGui(entities, nodeFlags, nodeClicked, player, &sun, &gravity, &enableDirLight);
        updatePlayer(scene, window, &input, player);
        updateRigidBodies(scene);
        processAnimators(scene, wrenchOffset);
        updateCamera(scene, player);
        setViewProjection(scene);
        drawPickingScene(scene, pickingFBO, pickingShader);
        if (isPicking) {
            // checkPicker(pickPosition, &windowData, entities, nodeClicked);
        }

        drawScene(scene, nodeClicked);
        // ImGui::Render();
        // ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    exitProgram(0);
    return 0;
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

void onScreenChanged(GLFWwindow* window, int width, int height) {
    Scene* scene = (Scene*)glfwGetWindowUserPointer(window);
    glViewport(0, 0, width, height);
    scene->windowData.width = width;
    scene->windowData.height = height;

    for (int i = 0; i < scene->cameras.size(); i++) {
        scene->cameras[i]->aspectRatio = (float)width / height;
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
    /* ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext(); */
    glfwTerminate();
    exit(code);
}