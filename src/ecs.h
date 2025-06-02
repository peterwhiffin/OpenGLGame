#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "forward.h"
#include "physics.h"

constexpr uint32_t INVALID_ID = 0xFFFFFFFF;
struct Player;

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

struct EntityGroup {
    uint32_t nextEntityID = 1;

    std::vector<Entity> entities;
    std::vector<Transform> transforms;
    std::vector<MeshRenderer> meshRenderers;
    std::vector<Animator> animators;
    std::vector<RigidBody> rigidbodies;
    std::vector<PointLight> pointLights;
    std::vector<SpotLight> spotLights;
    std::vector<Camera> cameras;
    std::vector<Player> players;

    std::unordered_set<uint32_t> movingRigidbodies;

    std::unordered_map<uint32_t, size_t> entityIndexMap;
    std::unordered_map<uint32_t, size_t> transformIndexMap;
    std::unordered_map<uint32_t, size_t> meshRendererIndexMap;
    std::unordered_map<uint32_t, size_t> rigidbodyIndexMap;
    std::unordered_map<uint32_t, size_t> animatorIndexMap;
    std::unordered_map<uint32_t, size_t> pointLightIndexMap;
    std::unordered_map<uint32_t, size_t> spotLightIndexMap;
    std::unordered_map<uint32_t, size_t> cameraIndexMap;
    std::unordered_map<uint32_t, size_t> playerIndexMap;
};

uint32_t createEntityFromModel(EntityGroup* scene, PhysicsScene* physicsScene, ModelNode* node, uint32_t parentEntityID, bool addColliders, uint32_t rootEntity, bool first, bool isDynamic);
uint32_t getEntityID(EntityGroup* scene);
Entity* getNewEntity(EntityGroup* scene, std::string name, uint32_t id = -1, bool createTransform = true);

Transform* addTransform(EntityGroup* scene, uint32_t entityID);
MeshRenderer* addMeshRenderer(EntityGroup* scene, uint32_t entityID);
RigidBody* addRigidbody(EntityGroup* scene, uint32_t entityID);
Player* addPlayer(EntityGroup* scene, uint32_t entityID);
Animator* addAnimator(EntityGroup* scene, uint32_t entityID, Model* model);
Animator* addAnimator(EntityGroup* scene, uint32_t entityID, std::vector<Animation*> animations);
Camera* addCamera(EntityGroup* scene, uint32_t entityID);
PointLight* addPointLight(EntityGroup* scene, uint32_t entityID);
SpotLight* addSpotLight(EntityGroup* scene, uint32_t entityID);

Entity* getEntity(EntityGroup* scene, const uint32_t entityID);
Transform* getTransform(EntityGroup* scene, const uint32_t entityID);
MeshRenderer* getMeshRenderer(EntityGroup* scene, const uint32_t entityID);
RigidBody* getRigidbody(EntityGroup* scene, const uint32_t entityID);
Player* getPlayer(EntityGroup* scene, const uint32_t entityID);
Animator* getAnimator(EntityGroup* scene, const uint32_t entityID);
PointLight* getPointLight(EntityGroup* scene, const uint32_t entityID);
SpotLight* getSpotLight(EntityGroup* scene, const uint32_t entityID);
Camera* getCamera(EntityGroup* scene, const uint32_t entityID);

void removeMeshRenderer(EntityGroup* scene, uint32_t entityID);
void removeAnimator(EntityGroup* scene, uint32_t entityID);
void removeRigidbody(EntityGroup* scene, uint32_t entityID, JPH::BodyInterface* bodyInterface = nullptr);
void removePlayer(EntityGroup* scene, uint32_t entityID);
void removeSpotLight(EntityGroup* scene, uint32_t entityID);
void removePointLight(EntityGroup* scene, uint32_t entityID);
void destroyEntity(EntityGroup* scene, uint32_t entityID);

void setRigidbodyMoving(EntityGroup* scene, uint32_t getEntityID);
void setRigidbodyNonMoving(EntityGroup* scene, uint32_t getEntityID);

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