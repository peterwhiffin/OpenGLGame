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

void exitProgram(RenderState* renderer, Resources* resources) {
    deleteBuffers(renderer, resources);
    glfwTerminate();
    exit(0);
}

void updateTime(Scene* scene) {
    scene->currentFrame = glfwGetTime();
    scene->deltaTime = scene->currentFrame - scene->lastFrame;
    scene->lastFrame = scene->currentFrame;
}

#ifdef PETES_EDITOR

void updateSceneEditor(Scene* scene, Resources* resources, RenderState* renderer, EditorState* editor) {
    if (editor->playing) {
        updatePhysics(scene);
        updatePlayer(scene, resources, renderer);
        updatePhysicsBodyPositions(scene);
        updateAnimators(scene);
        updateCamera(scene);
    } else {
        updateEditor(scene, resources, renderer, editor);
    }
}

int main() {
    Scene* scene = new Scene();
    Resources* resources = new Resources();
    RenderState* renderer = new RenderState();
    InputActions* inputActions = new InputActions();
    scene->input = inputActions;

    EditorState* editor = new EditorState();

    createContext(scene, renderer);
    loadEditorShaders(renderer);
    loadShaders(renderer);
    loadResources(resources, renderer);
    initPhysics(scene);
    initRenderer(renderer, scene);
    initRendererEditor(renderer);
    initEditor(editor, renderer->window);
    loadFirstFoundScene(scene, resources);

    scene->currentFrame = static_cast<float>(glfwGetTime());
    scene->lastFrame = scene->currentFrame;

    while (!glfwWindowShouldClose(renderer->window)) {
        glfwPollEvents();
        updateTime(scene);
        updateInput(inputActions, renderer->window);
        updateAndDrawEditor(scene, renderer, resources, editor);
        updateSceneEditor(scene, resources, renderer, editor);
        updateBufferData(renderer, scene);
        drawPickingScene(renderer, scene);
        renderScene(renderer, scene);
        renderDebug(renderer);
        glfwSwapBuffers(renderer->window);
    }

    destroyEditor();
    exitProgram(renderer, resources);
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