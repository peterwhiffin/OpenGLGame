#include "camera.h"
#include "scene.h"
#include "transform.h"

void updateCamera(Scene* scene, RenderState* renderer) {
    Camera* camera = scene->cameras[0];
    uint32_t cameraID = camera->entityID;
    uint32_t cameraTargetID = scene->player->cameraController->cameraTargetEntityID;
    vec3 newPosition = getPosition(scene, cameraTargetID);
    quat newRotation = getRotation(scene, cameraTargetID);

    setPosition(scene, cameraID, newPosition);
    setRotation(scene, cameraID, newRotation);

    camera->aspectRatio = (float)renderer->windowData.viewportWidth / renderer->windowData.viewportHeight;

    renderer->matricesUBOData.view = mat4::sLookAt(newPosition, newPosition + forward(scene, cameraID), up(scene, cameraID));
    renderer->matricesUBOData.projection = mat4::sPerspective(camera->fovRadians, camera->aspectRatio, camera->nearPlane, camera->farPlane);
}