#include "component.h"
#include "loader.h"
#include "sceneloader.h"
#include "input.h"
#include "camera.h"
#include "physics.h"
#include "animation.h"
#include "player.h"
#include "renderer.h"
#include "transform.h"
#include "shader.h"
#include "editor.h"

void exitProgram(Scene* scene, int code) {
    deleteBuffers(scene);
    destroyEditor();
    glfwTerminate();
    exit(code);
}

void updateTime(Scene* scene) {
    scene->currentFrame = static_cast<float>(glfwGetTime());
    scene->deltaTime = scene->currentFrame - scene->lastFrame;
    scene->lastFrame = scene->currentFrame;
}

int main() {
    Scene* scene = new Scene();
    createContext(scene);
    loadShaders(scene);
    loadResources(scene);
    initPhysics(scene);
    loadScene(scene);
    initRenderer(scene);
    initEditor(scene);

    scene->currentFrame = static_cast<float>(glfwGetTime());
    scene->lastFrame = scene->currentFrame;

    while (!glfwWindowShouldClose(scene->window)) {
        glfwPollEvents();
        updateTime(scene);
        updatePhysics(scene);
        updateInput(scene);
        updatePlayer(scene);
        updateAnimators(scene);
        updateCamera(scene);
        renderScene(scene);
        drawEditor(scene);
        glfwSwapBuffers(scene->window);
    }

    exitProgram(scene, 0);
    return 0;
}