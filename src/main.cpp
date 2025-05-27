#include <glad/glad.h>
#include <glfw/glfw3.h>

#include "loader.h"
#include "sceneloader.h"
#include "scene.h"
#include "renderer.h"
#include "shader.h"
#include "physics.h"
#include "input.h"
#include "player.h"
#include "animation.h"
#include "camera.h"
#include "editor.h"

void exitProgram(RenderState* renderer, Resources* resources, int code) {
    deleteBuffers(renderer, resources);
    destroyEditor();
    glfwTerminate();
    exit(code);
}

void updateTime(Scene* scene) {
    scene->currentFrame = glfwGetTime();
    scene->deltaTime = scene->currentFrame - scene->lastFrame;
    scene->lastFrame = scene->currentFrame;
}

int main() {
    Scene* scene = new Scene();
    Resources* resources = new Resources();
    RenderState* renderer = new RenderState();
    EditorState* editor = new EditorState();
    InputActions* inputActions = new InputActions();
    scene->input = inputActions;

    createContext(scene, renderer);
    loadShaders(renderer);
    loadResources(scene, resources, renderer);
    initPhysics(scene);
    loadScene(scene, resources);
    initRenderer(renderer, scene, editor);
    initEditor(editor, renderer->window);

    scene->currentFrame = static_cast<float>(glfwGetTime());
    scene->lastFrame = scene->currentFrame;

    while (!glfwWindowShouldClose(renderer->window)) {
        glfwPollEvents();
        updateTime(scene);
        updatePhysics(scene);
        updateInput(inputActions, renderer->window);
        updatePlayer(scene, resources, renderer);
        updateAnimators(scene);
        updateCamera(scene, renderer);
        renderScene(renderer, scene, editor);
        drawEditor(scene, renderer, resources, editor);
        glfwSwapBuffers(renderer->window);
    }

    exitProgram(renderer, resources, 0);
    return 0;
}