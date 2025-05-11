#pragma once
#include "component.h"

constexpr glm::vec3 kVectorRight = glm::vec3(1.0f, 0.0f, 0.0f);
constexpr glm::vec3 kVectorUp = glm::vec3(0.0f, 1.0f, 0.0f);
constexpr glm::vec3 kVectorForward = glm::vec3(0.0f, 0.0f, -1.0f);

glm::vec3 right(Scene* scene, uint32_t entityID);
glm::vec3 up(Scene* scene, uint32_t entityID);
glm::vec3 forward(Scene* scene, uint32_t entityID);

glm::vec3 getLocalPosition(Scene* scene, uint32_t entityID);
glm::quat getLocalRotation(Scene* scene, uint32_t entityID);
glm::vec3 getLocalScale(Scene* scene, uint32_t entityID);
glm::vec3 getPosition(Scene* scene, uint32_t entityID);
glm::quat getRotation(Scene* scene, uint32_t entityID);
glm::vec3 getScale(Scene* scene, uint32_t entityID);

void updateTransformMatrices(Scene* scene, Transform* transform);
void setLocalPosition(Scene* scene, uint32_t entityID, glm::vec3 localPosition);
void setLocalRotation(Scene* scene, uint32_t entityID, glm::quat localRotation);
void setLocalScale(Scene* scene, uint32_t entityID, glm::vec3 localScale);
void setPosition(Scene* scene, uint32_t entityID, glm::vec3 position);
void setRotation(Scene* scene, uint32_t entityID, glm::quat rotation);
void setScale(Scene* scene, uint32_t entityID, glm::vec3 scale);
void removeParent(Scene* scene, uint32_t entityID);
void setParent(Scene* scene, uint32_t child, uint32_t parent);

glm::vec3 scaleFromMatrix(glm::mat4& matrix);
glm::quat quatFromMatrix(glm::mat4& matrix);