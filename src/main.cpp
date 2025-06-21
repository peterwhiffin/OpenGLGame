#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <stdlib.h>

#include "sceneloader.h"
#include "miniaudio.h"
#include "soloud.h"
#include "soloud_wav.h"
#include "loader.h"
#include "scene.h"
#include "renderer.h"
#include "shader.h"
#include "physics.h"
#include "input.h"
#include "player.h"
#include "animation.h"
#include "camera.h"
#include "editor.h"

static void exitProgram(RenderState* renderer, Resources* resources) {
    deleteBuffers(renderer, resources);
    glfwTerminate();
    exit(0);
}

static void updateTime(Scene* scene) {
    scene->currentFrame = glfwGetTime();
    scene->deltaTime = scene->currentFrame - scene->lastFrame;
    scene->lastFrame = scene->currentFrame;
}

#ifdef PETES_EDITOR

static void updateSceneEditor(Scene* scene, Resources* resources, RenderState* renderer, EditorState* editor) {
    if (editor->playing) {
        updatePhysics(scene);
        updatePlayer(scene, resources, renderer);
        updatePhysicsBodyPositions(scene);
        updateAnimators(&scene->entities, scene->deltaTime);
        updateCamera(scene);
    } else {
        updateEditor(scene, resources, renderer, editor);
    }
}

int main() {
    Scene* scene = new Scene();
    Resources* resources = new Resources();
    RenderState* renderer = new RenderState();
    EditorState* editor = new EditorState();
    InputActions* inputActions = new InputActions();
    scene->input = inputActions;

    createContext(renderer);
    loadEditorShaders(renderer);
    loadShaders(renderer);
    loadResources(resources, renderer);
    initPhysics(scene);
    initRenderer(renderer, scene);
    initRendererEditor(renderer);
    initEditor(editor, renderer->window);
    loadFirstFoundScene(scene, resources);

    SoLoud::Soloud* gSoloud = new SoLoud::Soloud();
    SoLoud::Wav* sound = new SoLoud::Wav();
    gSoloud->init();
    sound->load("x:/repos/openglgame/resources/sounds/Ambiance_Cicadas_Loop_Stereo.wav");
    gSoloud->play(*sound);

    scene->currentFrame = static_cast<float>(glfwGetTime());
    scene->lastFrame = scene->currentFrame;

    while (!glfwWindowShouldClose(renderer->window)) {
        glfwPollEvents();
        updateTime(scene);
        updateInput(inputActions, renderer->window);
        updateAndDrawEditor(scene, renderer, resources, editor);
        updateSceneEditor(scene, resources, renderer, editor);
        updateBufferData(renderer, scene);
        drawPickingScene(renderer, &scene->entities);
        renderScene(renderer, &scene->entities);
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

    createContext(renderer);
    loadShaders(renderer);
    loadResources(resources, renderer);
    initPhysics(scene);
    loadFirstFoundScene(scene, resources);
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
        updateAnimators(&scene->entities, scene->deltaTime);
        updateCamera(scene);
        updateBufferData(renderer, scene);
        renderScene(renderer, &scene->entities);
        glfwSwapBuffers(renderer->window);
    }

    exitProgram(renderer, resources);
    return 0;
}
#endif
