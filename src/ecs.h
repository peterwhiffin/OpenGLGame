#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include "forward.h"

constexpr uint32_t INVALID_ID = 0xFFFFFFFF;

enum ContextMenuType {
    WindowInspector,
    WindowHierarchy,
    WindowProject,
    ItemHierarchy,
    ItemInspector,
    ItemProject
};

struct Entity {
    uint32_t entityID;
    std::string name;
    bool isActive;
};

uint32_t createEntityFromModel(Scene* scene, ModelNode* node, uint32_t parentEntityID, bool addColliders, uint32_t rootEntity, bool first, bool isDynamic);
uint32_t getEntityID(Scene* scene);
Entity* getNewEntity(Scene* scene, std::string name, uint32_t id = -1, bool createTransform = true);

Transform* addTransform(Scene* scene, uint32_t entityID);
MeshRenderer* addMeshRenderer(Scene* scene, uint32_t entityID);
RigidBody* addRigidbody(Scene* scene, uint32_t entityID);
Animator* addAnimator(Scene* scene, uint32_t entityID, Model* model);
Animator* addAnimator(Scene* scene, uint32_t entityID, std::vector<Animation*> animations);
Camera* addCamera(Scene* scene, uint32_t entityID, float fov, float nearPlane, float farPlane);
PointLight* addPointLight(Scene* scene, uint32_t entityID);
SpotLight* addSpotLight(Scene* scene, uint32_t entityID);

Entity* getEntity(Scene* scene, const uint32_t entityID);
Transform* getTransform(Scene* scene, const uint32_t entityID);
MeshRenderer* getMeshRenderer(Scene* scene, const uint32_t entityID);
RigidBody* getRigidbody(Scene* scene, const uint32_t entityID);
Animator* getAnimator(Scene* scene, const uint32_t entityID);
PointLight* getPointLight(Scene* scene, const uint32_t entityID);
SpotLight* getSpotLight(Scene* scene, const uint32_t entityID);
Camera* getCamera(Scene* scene, const uint32_t entityID);

void removeMeshRenderer(Scene* scene, uint32_t entityID);
void removeAnimator(Scene* scene, uint32_t entityID);
void removeRigidbody(Scene* scene, uint32_t entityID);
void removeSpotLight(Scene* scene, uint32_t entityID);
void removePointLight(Scene* scene, uint32_t entityID);
void destroyEntity(Scene* scene, uint32_t entityID);

void setRigidbodyMoving(Scene* scene, uint32_t getEntityID);
void setRigidbodyNonMoving(Scene* scene, uint32_t getEntityID);

template <typename Component>
bool destroyComponent(std::vector<Component>& components, std::unordered_map<uint32_t, size_t>& indexMap, uint32_t entityID) {
    if (!indexMap.count(entityID)) {
        return false;
    }

    size_t indexToRemove = indexMap[entityID];
    size_t lastIndex = components.size() - 1;

    if (indexToRemove != lastIndex) {
        uint32_t lastID = components[lastIndex].entityID;
        std::swap(components[lastIndex], components[indexToRemove]);
        indexMap[lastID] = indexToRemove;
    }

    components.pop_back();
    indexMap.erase(entityID);
    return true;
}