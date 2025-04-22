#include "transform.h"
#include "camera.h"

void updateCamera(Scene* scene, Player* player) {
    setPosition(scene, scene->cameras[0]->entityID, glm::mix(getPosition(scene, scene->cameras[0]->entityID), getPosition(scene, player->cameraController->cameraTargetEntityID), 1.0f));
    setRotation(scene, scene->cameras[0]->entityID, glm::slerp(getRotation(scene, scene->cameras[0]->entityID), getRotation(scene, player->cameraController->cameraTargetEntityID), 1.0f));
}
void setViewProjection(Scene* scene) {
    Camera* camera = scene->cameras[0];
    glm::vec3 position = getPosition(scene, camera->entityID);
    camera->viewMatrix = glm::lookAt(position, position + forward(scene, camera->entityID), up(scene, camera->entityID));
    camera->projectionMatrix = glm::perspective(camera->fov, camera->aspectRatio, camera->nearPlane, camera->farPlane);
}
