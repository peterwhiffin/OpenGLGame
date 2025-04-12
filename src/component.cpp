#include <glm/ext/quaternion_float.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "component.h"
#include "renderer.h"

Entity::Entity() : transform(this) {}
Component::Component(Entity* newEntity) : entity(newEntity), transform(&newEntity->transform) {}
Transform::Transform(Entity* entity) : Component(entity) {}
MeshRenderer::MeshRenderer(Entity* entity, Mesh* mesh) : Component(entity), mesh(mesh) {}
Camera::Camera(Entity* entity, float fov, float aspectRatio, float nearPlane, float farPlane) : Component(entity), fov(fov), aspectRatio(aspectRatio), nearPlane(nearPlane), farPlane(farPlane) {}
CameraController::CameraController(Entity* entity, Camera& camera) : Component(entity), camera(camera) {}

glm::vec3 right(Transform* transform) {
    return QuaternionByVector3(transform->rotation, glm::vec3(1.0f, 0.0f, 0.0f));
}

glm::vec3 up(Transform* transform) {
    return QuaternionByVector3(transform->rotation, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::vec3 forward(Transform* transform) {
    return QuaternionByVector3(transform->rotation, glm::vec3(0.0f, 0.0f, -1.0f));
}

glm::vec3 QuaternionByVector3(glm::quat rotation, glm::vec3 point) {
    float num = rotation.x + rotation.x;
    float num2 = rotation.y + rotation.y;
    float num3 = rotation.z + rotation.z;

    float num4 = rotation.x * num;
    float num5 = rotation.y * num2;
    float num6 = rotation.z * num3;

    float num7 = rotation.x * num2;
    float num8 = rotation.x * num3;
    float num9 = rotation.y * num3;

    float num10 = rotation.w * num;
    float num11 = rotation.w * num2;
    float num12 = rotation.w * num3;

    glm::vec3 result = glm::vec3(0.0f, 0.0f, 0.0f);

    result.x = (1.0f - (num5 + num6)) * point.x + (num7 - num12) * point.y + (num8 + num11) * point.z;
    result.y = (num7 + num12) * point.x + (1.0f - (num4 + num6)) * point.y + (num9 - num10) * point.z;
    result.z = (num8 - num11) * point.x + (num9 + num10) * point.y + (1.0f - (num4 + num5)) * point.z;

    return result;
}