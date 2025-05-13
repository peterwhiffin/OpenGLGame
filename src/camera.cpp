#include "transform.h"
#include "camera.h"
#include "player.h"

void updateCamera(Scene* scene) {
    Camera* camera = scene->cameras[0];
    uint32_t cameraID = camera->entityID;

    setPosition(scene, cameraID, lerp(getPosition(scene, cameraID), getPosition(scene, scene->player->cameraController->cameraTargetEntityID), 1.0f));
    setRotation(scene, cameraID, getRotation(scene, cameraID).SLERP(getRotation(scene, scene->player->cameraController->cameraTargetEntityID), 1.0f));

    vec3 position = getPosition(scene, cameraID);
    scene->matricesUBOData.view = mat4::sLookAt(position, position + forward(scene, cameraID), up(scene, cameraID));
    scene->matricesUBOData.projection = mat4::sPerspective(camera->fovRadians, camera->aspectRatio, camera->nearPlane, camera->farPlane);
}