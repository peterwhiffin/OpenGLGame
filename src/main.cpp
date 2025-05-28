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
    glfwTerminate();
    exit(code);
}

void updateTime(Scene* scene) {
    scene->currentFrame = glfwGetTime();
    scene->deltaTime = scene->currentFrame - scene->lastFrame;
    scene->lastFrame = scene->currentFrame;
}

#ifdef PETES_EDITOR
int main() {
    Scene* scene = new Scene();
    Resources* resources = new Resources();
    RenderState* renderer = new RenderState();
    InputActions* inputActions = new InputActions();
    scene->input = inputActions;

    EditorState* editor = new EditorState();

    createContext(scene, renderer);
    loadShaders(renderer);
    loadResources(resources, renderer);
    initPhysics(scene);
    loadScene(scene, resources);
    initRenderer(renderer, scene);
    initRendererEditor(renderer);
    initEditor(editor, renderer->window);

    scene->currentFrame = static_cast<float>(glfwGetTime());
    scene->lastFrame = scene->currentFrame;

    while (!glfwWindowShouldClose(renderer->window)) {
        glfwPollEvents();
        updateTime(scene);
        updateInput(inputActions, renderer->window);
        updatePhysics(scene);
        updatePlayer(scene, resources, renderer);
        updatePhysicsBodyPositions(scene);
        updateAnimators(scene);
        updateCamera(scene);
        updateBufferData(renderer, scene);
        drawPickingScene(renderer, scene);
        renderScene(renderer, scene);
        renderDebug(renderer);
        drawEditor(scene, renderer, resources, editor);
        glfwSwapBuffers(renderer->window);
    }

    destroyEditor();
    exitProgram(renderer, resources, 0);
    return 0;
}

#else
int main() {
    Scene* scene = new Scene();
    Resources* resources = new Resources();
    RenderState* renderer = new RenderState();
    InputActions* inputActions = new InputActions();
    scene->input = inputActions;

    createContext(scene, renderer);
    loadShaders(renderer);
    loadResources(resources, renderer);
    initPhysics(scene);
    loadScene(scene, resources);
    initRenderer(renderer, scene);

    scene->currentFrame = static_cast<float>(glfwGetTime());
    scene->lastFrame = scene->currentFrame;

    while (!glfwWindowShouldClose(renderer->window)) {
        glfwPollEvents();
        updateTime(scene);
        updateInput(inputActions, renderer->window);
        updatePhysics(scene);
        updatePlayer(scene, resources, renderer);
        updatePhysicsBodyPositions(scene);
        updateAnimators(scene);
        updateCamera(scene);
        updateBufferData(renderer, scene);
        renderScene(renderer, scene);
        glfwSwapBuffers(renderer->window);
    }

    exitProgram(renderer, resources, 0);
    return 0;
}
#endif