#include "camera.h"
#include "scene.h"
#include "transform.h"

void updateEditorCamera(EditorState* editor, Scene* scene, RenderState* renderer) {
    EntityGroup* entities = &scene->entities;
    InputActions* input = scene->input;
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

    Camera* cam = &entities->cameras[0];
    Transform* transform = getTransform(entities, cam->entityID);

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

    setRotation(entities, transform->entityID, quat::sEulerAngles(cameraTargetRotation));

    vec3 moveDir = vec3(0.0f, 0.0f, 0.0f);
    vec3 forwardDir = transformForward(entities, transform->entityID);
    vec3 rightDir = transformRight(entities, transform->entityID);
    moveDir += input->movement.y * forwardDir + input->movement.x * rightDir;
    vec3 finalMove = moveDir * editor->cameraController.moveSpeed * scene->deltaTime;
    vec3 currentPos = getPosition(entities, transform->entityID);
    setPosition(entities, transform->entityID, currentPos + finalMove);
}

void updateCamera(Scene* scene) {
    EntityGroup* entities = &scene->entities;
    Camera* camera = &entities->cameras[0];

    uint32_t cameraID = camera->entityID;
    uint32_t cameraTargetID = entities->players[0].cameraController.cameraTargetEntityID;

    vec3 targetPosition = getPosition(entities, cameraTargetID);
    quat targetRotation = getRotation(entities, cameraTargetID);

    setPosition(entities, cameraID, targetPosition);
    setRotation(entities, cameraID, targetRotation);
}