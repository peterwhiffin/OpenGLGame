#include "camera.h"
#include "scene.h"
#include "transform.h"

void updateCamera(Scene* scene) {
    Camera* camera = scene->cameras[0];
    uint32_t cameraID = camera->entityID;
    uint32_t cameraTargetID = scene->player->cameraController->cameraTargetEntityID;

    vec3 targetPosition = getPosition(scene, cameraTargetID);
    quat targetRotation = getRotation(scene, cameraTargetID);

    setPosition(scene, cameraID, targetPosition);
    setRotation(scene, cameraID, targetRotation);
}