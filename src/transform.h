#pragma once
// #include "forward.h"
#include "utils/mathutils.h"

struct Scene;
struct EntityGroup;

struct Transform {
    uint32_t entityID;
    uint32_t parentEntityID;
    vec3 localPosition = vec3(0.0f, 0.0f, 0.0f);
    quat localRotation = quat(0.0f, 0.0f, 0.0f, 1.0f);
    vec3 localScale = vec3(1.0f, 1.0f, 1.0f);
    mat4 worldTransform = mat4::sIdentity();
    std::vector<uint32_t> childEntityIds;
};

vec3 transformRight(EntityGroup* scene, uint32_t entityID);
vec3 transformUp(EntityGroup* scene, uint32_t entityID);
vec3 transformForward(EntityGroup* scene, uint32_t entityID);

vec3 getLocalPosition(EntityGroup* scene, uint32_t entityID);
quat getLocalRotation(EntityGroup* scene, uint32_t entityID);
vec3 getLocalScale(EntityGroup* scene, uint32_t entityID);
vec3 getPosition(EntityGroup* scene, uint32_t entityID);
quat getRotation(EntityGroup* scene, uint32_t entityID);
vec3 getScale(EntityGroup* scene, uint32_t entityID);

void updateTransformMatrices(EntityGroup* scene, Transform* transform);
void setLocalPosition(EntityGroup* scene, uint32_t entityID, vec3 localPosition);
void setLocalRotation(EntityGroup* scene, uint32_t entityID, quat localRotation);
void setLocalScale(EntityGroup* scene, uint32_t entityID, vec3 localScale);
void setPosition(EntityGroup* scene, uint32_t entityID, vec3 position);
void setRotation(EntityGroup* scene, uint32_t entityID, quat rotation);
void setScale(EntityGroup* scene, uint32_t entityID, vec3 scale);
void removeParent(EntityGroup* scene, uint32_t entityID);
void setParent(EntityGroup* scene, uint32_t child, uint32_t parent);

vec3 scaleFromMatrix(mat4& matrix);
quat quatFromMatrix(mat4& matrix);