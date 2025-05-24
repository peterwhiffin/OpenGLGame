#pragma once
// #include "forward.h"
#include "utils/mathutils.h"

struct Scene;

struct Transform {
    uint32_t entityID;
    uint32_t parentEntityID;
    vec3 localPosition = vec3(0.0f, 0.0f, 0.0f);
    quat localRotation = quat(0.0f, 0.0f, 0.0f, 1.0f);
    vec3 localScale = vec3(1.0f, 1.0f, 1.0f);
    mat4 worldTransform = mat4::sIdentity();
    std::vector<uint32_t> childEntityIds;
};

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