#pragma once
#include "component.h"

vec3 right(Scene* scene, uint32_t entityID);
vec3 up(Scene* scene, uint32_t entityID);
vec3 forward(Scene* scene, uint32_t entityID);

vec3 getLocalPosition(Scene* scene, uint32_t entityID);
quat getLocalRotation(Scene* scene, uint32_t entityID);
vec3 getLocalScale(Scene* scene, uint32_t entityID);
vec3 getPosition(Scene* scene, uint32_t entityID);
quat getRotation(Scene* scene, uint32_t entityID);
vec3 getScale(Scene* scene, uint32_t entityID);

void updateTransformMatrices(Scene* scene, Transform* transform);
void setLocalPosition(Scene* scene, uint32_t entityID, vec3 localPosition);
void setLocalRotation(Scene* scene, uint32_t entityID, quat localRotation);
void setLocalScale(Scene* scene, uint32_t entityID, vec3 localScale);
void setPosition(Scene* scene, uint32_t entityID, vec3 position);
void setRotation(Scene* scene, uint32_t entityID, quat rotation);
void setScale(Scene* scene, uint32_t entityID, vec3 scale);
void removeParent(Scene* scene, uint32_t entityID);
void setParent(Scene* scene, uint32_t child, uint32_t parent);

vec3 scaleFromMatrix(mat4& matrix);
quat quatFromMatrix(mat4& matrix);