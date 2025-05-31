#include "camera.h"
#include "scene.h"
#include "transform.h"

void updateEditorCamera(EditorState* editor, Scene* scene, RenderState* renderer) {
    InputActions* input = scene->input;
    /*     if (input->menu) {
            if (scene->menuCanOpen) {
                scene->menuOpen = !scene->menuOpen;
                scene->menuCanOpen = false;

                glfwSetInputMode(renderer->window, GLFW_CURSOR, scene->menuOpen ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
            }
        } else {
            scene->menuCanOpen = true;
        }

        if (scene->menuOpen) {
            return;
        } */

    if (input->altFire) {
        if (editor->mouseInViewport && !editor->cameraActive) {
            glfwSetInputMode(renderer->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            editor->cameraActive = true;
        }
    } else {
        if (editor->cameraActive) {
            glfwSetInputMode(renderer->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            editor->cameraActive = false;
        }

        return;
    }

    Camera* cam = scene->cameras[0];
    Transform* transform = getTransform(scene, cam->entityID);

    float xOffset = input->lookX * editor->cameraController.sensitivity;
    float yOffset = input->lookY * editor->cameraController.sensitivity;
    float pitch = editor->cameraController.pitch;
    float yaw = editor->cameraController.yaw;

    yaw -= xOffset;
    pitch -= yOffset;

    if (pitch > 89.0f) {
        pitch = 89.0f;
    }

    if (pitch < -89.0f) {
        pitch = -89.0f;
    }

    editor->cameraController.pitch = pitch;
    editor->cameraController.yaw = yaw;

    vec3 cameraTargetRotation = vec3(JPH::DegreesToRadians(editor->cameraController.pitch), JPH::DegreesToRadians(editor->cameraController.yaw), 0.0f);

    setRotation(scene, transform->entityID, quat::sEulerAngles(cameraTargetRotation));

    vec3 moveDir = vec3(0.0f, 0.0f, 0.0f);
    vec3 forwardDir = forward(scene, transform->entityID);
    vec3 rightDir = right(scene, transform->entityID);
    moveDir += input->movement.y * forwardDir + input->movement.x * rightDir;
    vec3 finalMove = moveDir * editor->cameraController.moveSpeed * scene->deltaTime;
    vec3 currentPos = getPosition(scene, transform->entityID);
    setPosition(scene, transform->entityID, currentPos + finalMove);
}

void updateCamera(Scene* scene) {
    Camera* camera = scene->cameras[0];
    uint32_t cameraID = camera->entityID;
    uint32_t cameraTargetID = scene->player->cameraController->cameraTargetEntityID;

    vec3 targetPosition = getPosition(scene, cameraTargetID);
    quat targetRotation = getRotation(scene, cameraTargetID);

    setPosition(scene, cameraID, targetPosition);
    setRotation(scene, cameraID, targetRotation);
}