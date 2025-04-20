#pragma once
#include <glm/ext/quaternion_float.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "component.h"

glm::vec3 right(Transform* transform);
glm::vec3 up(Transform* transform);
glm::vec3 forward(Transform* transform);
glm::vec3 QuaternionByVector3(glm::quat rotation, glm::vec3 point);

void updateTransformMatrices(Transform* transform);

glm::vec3 getPosition(Transform* transform);
glm::quat getRotation(Transform* transform);
glm::vec3 getScale(Transform* transform);
glm::quat quatFromMatrix(glm::mat4& matrix);

void setLocalPosition(Transform* transform, glm::vec3 localPosition);
void setLocalRotation(Transform* transform, glm::quat localRotation);
void setLocalScale(Transform* transform, glm::vec3 localScale);
void setPosition(Transform* transform, glm::vec3 position);
void setRotation(Transform* transform, glm::quat rotation);
void setScale(Transform* transform, glm::vec3 scale);
void setParent(Transform* child, Transform* parent);
void removeParent(Transform* transform);
