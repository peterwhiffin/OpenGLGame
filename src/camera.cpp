#include "transform.h"
#include "camera.h"

void updateCamera(Player* player) {
    setPosition(player->cameraController->camera->transform, glm::mix(getPosition(player->cameraController->camera->transform), getPosition(player->cameraController->cameraTarget), 1.0f));
    setRotation(player->cameraController->camera->transform, glm::slerp(getRotation(player->cameraController->camera->transform), getRotation(player->cameraController->cameraTarget), 1.0f));
}
void setViewProjection(Camera* camera) {
    glm::vec3 position = getPosition(camera->transform);
    camera->viewMatrix = glm::lookAt(position, position + forward(camera->transform), up(camera->transform));
    camera->projectionMatrix = glm::perspective(camera->fov, camera->aspectRatio, camera->nearPlane, camera->farPlane);
}
