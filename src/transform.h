#pragma once
#include "component.h"

glm::vec3 right(Transform* transform);
glm::vec3 up(Transform* transform);
glm::vec3 forward(Transform* transform);

glm::vec3 getLocalPosition(Transform* transform);
glm::quat getLocalRotation(Transform* transform);
glm::vec3 getLocalScale(Transform* transform);
glm::vec3 getPosition(Transform* transform);
glm::quat getRotation(Transform* transform);
glm::vec3 getScale(Transform* transform);

void setLocalPosition(Transform* transform, glm::vec3 localPosition);
void setLocalRotation(Transform* transform, glm::quat localRotation);
void setLocalScale(Transform* transform, glm::vec3 localScale);
void setPosition(Transform* transform, glm::vec3 position);
void setRotation(Transform* transform, glm::quat rotation);
void setScale(Transform* transform, glm::vec3 scale);
void removeParent(Transform* transform);
void setParent(Transform* child, Transform* parent);