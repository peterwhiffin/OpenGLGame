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
        glUniform3fv(glGetUniformLocation(shader, (base + ".color").c_str()), 1, glm::value_ptr(pointLight->color));
        glUniform1f(glGetUniformLocation(shader, (base + ".brightness").c_str()), pointLight->brightness);
        glUniform1ui(glGetUniformLocation(shader, (base + ".isActive").c_str()), pointLight->isActive);
    }

    /* glUseProgram(scene->ssaoShader);
    for (int i = 0; i < scene->ssaoKernel.size(); i++) {
        glUniform3fv(glGetUniformLocation(shader, ("u_kernel[" + std::to_string(i) + "]").c_str()), 1, glm::value_ptr(scene->ssaoKernel[i]));
    } */
}

void loadDefaultScene(Scene* scene) {
    for (int i = 0; i < 10; i++) {
        Entity* pointLightEntity = getNewEntity(scene, "PointLight");
        PointLight* pointLight = addPointLight(scene, pointLightEntity->id);
        setPosition(scene, pointLightEntity->id, glm::vec3(2.0f + i / 2, 3.0f, 1.0f + i / 2));
        pointLight->color = glm::vec3(1.0f);
        pointLight->isActive = true;
        pointLight->brightness = 1.0f;
    }

    uint32_t wrenchEntity = createEntityFromModel(scene, scene->wrench->rootNode, INVALID_ID, false);
    uint32_t levelEntity = createEntityFromModel(scene, scene->testRoom->rootNode, INVALID_ID, true);
    uint32_t trashCanEntity = createEntityFromModel(scene, scene->trashcanModel->rootNode, INVALID_ID, true);

    addAnimator(scene, wrenchEntity, scene->wrench);
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

    Player* player = createPlayer(scene);
    setParent(scene, wrenchEntity, player->cameraController->cameraTargetEntityID);
    setLocalRotation(scene, wrenchEntity, glm::quat(glm::vec3(glm::radians(0.0f), glm::radians(180.0f), 0.0f)));
    setLocalPosition(scene, wrenchEntity, scene->wrenchOffset);
}

int main() {
    unsigned int pickingShader;
    unsigned int pickingFBO, pickingRBO, pickingTexture;
    unsigned int whiteTexture;
    unsigned int blackTexture;
    unsigned int blueTexture;
    unsigned int nodeclicked = INVALID_ID;
    unsigned char whitePixel[4] = {255, 255, 255, 255};
    unsigned char blackPixel[4] = {0, 0, 0, 255};
    unsigned char bluePixel[4] = {0, 0, 255, 255};

    std::string scenePath;
    Scene* scene = new Scene;
    scene->windowData.width = 800;
    scene->windowData.height = 600;
    GLFWwindow* window = createContext(scene);
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
    black.path = "black";
    blue.path = "blue";

    scene->textures.push_back(black);
    scene->textures.push_back(white);
    scene->textures.push_back(blue);

    scene->testRoom = loadModel(scene, "../resources/models/testroom/testroom.gltf", &scene->textures, scene->lightingShader, true);
    scene->wrench = loadModel(scene, "../resources/models/wrench/wrench.gltf", &scene->textures, scene->lightingShader, true);
    scene->trashcanModel = loadModel(scene, "../resources/models/trashcan/trashcan.gltf", &scene->textures, scene->lightingShader, true);

    if (findLastScene(&scenePath)) {
        loadScene(scene, scenePath);
    } else {
        loadDefaultScene(scene);
    }

    createPickingFBO(scene, &pickingFBO, &pickingRBO, &pickingTexture);
    createDepthPrePassBuffer(scene);
    createForwardBuffer(scene);
    createBlurBuffers(scene);
    createFullScreenQuad(scene);
    generateSSAOKernel(scene);

    scene->depthShader = loadShader("../src/shaders/depthprepassshader.vs", "../src/shaders/depthprepassshader.fs");
    scene->lightingShader = loadShader("../src/shaders/pbrlitshader.vs", "../src/shaders/pbrlitshader.fs");
    scene->blurShader = loadShader("../src/shaders/gaussianblurshader.vs", "../src/shaders/gaussianblurshader.fs");
    scene->postProcessShader = loadShader("../src/shaders/postprocessshader.vs", "../src/shaders/postprocessshader.fs");
    pickingShader = loadShader("../src/shaders/pickingshader.vs", "../src/shaders/pickingshader.fs");

    initializeLights(scene, scene->lightingShader);

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
        updateCamera(scene, scene->player);
        // drawDepthPrePass(scene);
        drawScene(scene, nodeclicked);
        drawBlurPass(scene);
        drawFullScreenQuad(scene);

        if (scene->menuOpen) {
            drawDebug(scene, nodeFlags, nodeclicked, scene->player);
        }

        glfwSwapBuffers(window);
    }

    exitProgram(0);
    return 0;
}