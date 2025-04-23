#include "transform.h"
#include "camera.h"
#include "player.h"

void updateCamera(Scene* scene, Player* player) {
    Camera* camera = scene->cameras[0];
    uint32_t cameraID = camera->entityID;

    setPosition(scene, cameraID, glm::mix(getPosition(scene, cameraID), getPosition(scene, player->cameraController->cameraTargetEntityID), 1.0f));
    setRotation(scene, cameraID, glm::slerp(getRotation(scene, cameraID), getRotation(scene, player->cameraController->cameraTargetEntityID), 1.0f));

    glm::vec3 position = getPosition(scene, cameraID);
    camera->viewMatrix = glm::lookAt(position, position + forward(scene, cameraID), up(scene, cameraID));
    camera->projectionMatrix = glm::perspective(camera->fov, camera->aspectRatio, camera->nearPlane, camera->farPlane);
}