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

void initializeLights(Scene* scene, unsigned int shader) {
    glUseProgram(shader);
    glUniform3fv(glGetUniformLocation(shader, "dirLight.position"), 1, glm::value_ptr(scene->sun.position));
    glUniform3fv(glGetUniformLocation(shader, "dirLight.ambient"), 1, glm::value_ptr(scene->sun.ambient));
    glUniform3fv(glGetUniformLocation(shader, "dirLight.diffuse"), 1, glm::value_ptr(scene->sun.diffuse));
    glUniform3fv(glGetUniformLocation(shader, "dirLight.specular"), 1, glm::value_ptr(scene->sun.specular));

    int numLights = scene->pointLights.size();
    glUniform1i(glGetUniformLocation(shader, "numPointLights"), numLights);

    for (int i = 0; i < numLights; i++) {
        PointLight* pointLight = &scene->pointLights[i];
        std::string base = "pointLights[" + std::to_string(i) + "]";
        glUniform3fv(glGetUniformLocation(shader, (base + ".position").c_str()), 1, glm::value_ptr(getPosition(scene, pointLight->entityID)));
        glUniform3fv(glGetUniformLocation(shader, (base + ".ambient").c_str()), 1, glm::value_ptr(pointLight->ambient));
        glUniform3fv(glGetUniformLocation(shader, (base + ".diffuse").c_str()), 1, glm::value_ptr(pointLight->diffuse));
        glUniform3fv(glGetUniformLocation(shader, (base + ".specular").c_str()), 1, glm::value_ptr(pointLight->specular));
        glUniform1f(glGetUniformLocation(shader, (base + ".constant").c_str()), pointLight->constant);
        glUniform1f(glGetUniformLocation(shader, (base + ".linear").c_str()), pointLight->linear);
        glUniform1f(glGetUniformLocation(shader, (base + ".quadratic").c_str()), pointLight->quadratic);
        glUniform1f(glGetUniformLocation(shader, (base + ".brightness").c_str()), pointLight->brightness);
        glUniform1ui(glGetUniformLocation(shader, (base + ".isActive").c_str()), pointLight->isActive);
    }
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

    pointLightEntity = getNewEntity(scene, "PointLight2");
    pointLight = addPointLight(scene, pointLightEntity->id);
    setPosition(scene, pointLightEntity->id, glm::vec3(4.0f, 3.0f, -3.0f));
    pointLight->ambient = glm::vec3(0.05f);
    pointLight->diffuse = glm::vec3(0.8f);
    pointLight->specular = glm::vec3(1.0f);
    pointLight->constant = 1.0f;
    pointLight->linear = 0.09f;
    pointLight->quadratic = 0.032f;
    pointLight->isActive = 1;
    pointLight->brightness = 2.0f;

    createPickingFBO(scene, &pickingFBO, &pickingRBO, &pickingTexture);
    createHDRBuffer(scene);
    createFullScreenQuad(scene);

    scene->litForward = loadShader("../src/shaders/litshader.vs", "../src/shaders/litshader.fs");
    scene->postProcess = loadShader("../src/shaders/postprocessshader.vs", "../src/shaders/postprocessshader.fs");
    pickingShader = loadShader("../src/shaders/pickingshader.vs", "../src/shaders/pickingshader.fs");

    initializeLights(scene, scene->litForward);

    Model* testRoom = loadModel("../resources/models/testroom/testroom.gltf", &scene->textures, scene->litForward, true);
    Model* wrench = loadModel("../resources/models/wrench/wrench.gltf", &scene->textures, scene->litForward, true);
    scene->trashcanModel = loadModel("../resources/models/trashcan/trashcan.gltf", &scene->textures, scene->litForward, true);

    uint32_t levelEntity = createEntityFromModel(scene, testRoom->rootNode, INVALID_ID, true);
    uint32_t trashCanEntity = createEntityFromModel(scene, scene->trashcanModel->rootNode, INVALID_ID, true);
    uint32_t wrenchEntity = createEntityFromModel(scene, wrench->rootNode, INVALID_ID, false);

    addAnimator(scene, wrenchEntity, wrench);
    setPosition(scene, trashCanEntity, glm::vec3(1.0f, 3.0f, 2.0f));
    getBoxCollider(scene, trashCanEntity)->isActive = false;
    getBoxCollider(scene, getTransform(scene, trashCanEntity)->childEntityIds[0])->isActive = false;

    RigidBody* rb = addRigidbody(scene, trashCanEntity);
    rb->mass = 10.0f;
    rb->linearDrag = 3.0f;
    rb->friction = 10.0f;

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
        drawScene(scene, nodeclicked);
        drawFullScreenQuad(scene);

        if (scene->menuOpen) {
            drawDebug(scene, nodeFlags, nodeclicked, player);
        }

        glfwSwapBuffers(window);
    }

    exitProgram(0);
    return 0;
}