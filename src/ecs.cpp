#include "ecs.h"
#include "scene.h"
#include "loader.h"
#include "sceneloader.h"
#include "meshrenderer.h"

uint32_t getEntityID(EntityGroup* scene) {
    uint32_t newID = scene->nextEntityID++;

    while (scene->entityIndexMap.count(newID)) {
        newID = scene->nextEntityID++;
    }

    scene->entityIndexMap[newID] = 0;
    return newID;
}

Entity* getEntity(EntityGroup* scene, const uint32_t entityID) {
    return &scene->entities[scene->entityIndexMap[entityID]];
}

Transform* getTransform(EntityGroup* scene, const uint32_t entityID) {
    return &scene->transforms[scene->transformIndexMap[entityID]];
}

MeshRenderer* getMeshRenderer(EntityGroup* scene, const uint32_t entityID) {
    if (!scene->meshRendererIndexMap.count(entityID)) {
        return nullptr;
    }

    return &scene->meshRenderers[scene->meshRendererIndexMap[entityID]];
}

RigidBody* getRigidbody(EntityGroup* scene, const uint32_t entityID) {
    if (!scene->rigidbodyIndexMap.count(entityID)) {
        return nullptr;
    }

    return &scene->rigidbodies[scene->rigidbodyIndexMap[entityID]];
}

Animator* getAnimator(EntityGroup* scene, const uint32_t entityID) {
    if (!scene->animatorIndexMap.count(entityID)) {
        return nullptr;
    }

    return &scene->animators[scene->animatorIndexMap[entityID]];
}

Player* getPlayer(EntityGroup* scene, const uint32_t entityID) {
    if (!scene->playerIndexMap.count(entityID)) {
        return nullptr;
    }

    return &scene->players[scene->playerIndexMap[entityID]];
}

PointLight* getPointLight(EntityGroup* scene, const uint32_t entityID) {
    if (!scene->pointLightIndexMap.count(entityID)) {
        return nullptr;
    }

    return &scene->pointLights[scene->pointLightIndexMap[entityID]];
}

SpotLight* getSpotLight(EntityGroup* scene, const uint32_t entityID) {
    if (!scene->spotLightIndexMap.count(entityID)) {
        return nullptr;
    }

    return &scene->spotLights[scene->spotLightIndexMap[entityID]];
}

Camera* getCamera(EntityGroup* scene, const uint32_t entityID) {
    if (!scene->cameraIndexMap.count(entityID)) {
        return nullptr;
    }

    return &scene->cameras[scene->cameraIndexMap[entityID]];
}

Transform* addTransform(EntityGroup* scene, uint32_t entityID) {
    Transform transform;
    transform.entityID = entityID;
    transform.parentEntityID = INVALID_ID;
    size_t index = scene->transforms.size();
    scene->transforms.push_back(transform);
    scene->transformIndexMap[entityID] = index;
    return &scene->transforms[index];
}

Entity* getNewEntity(EntityGroup* scene, std::string name, uint32_t id, bool createTransform) {
    Entity entity;
    if (id == INVALID_ID) {
        entity.entityID = getEntityID(scene);
    } else {
        // registerEntityID(scene, id);
        entity.entityID = id;
    }

    entity.name = name;
    size_t index = scene->entities.size();
    scene->entities.push_back(entity);
    scene->entityIndexMap[entity.entityID] = index;
    if (createTransform) {
        addTransform(scene, entity.entityID);
    }
    return &scene->entities[index];
}

MeshRenderer* addMeshRenderer(EntityGroup* scene, uint32_t entityID) {
    MeshRenderer meshRenderer;
    meshRenderer.entityID = entityID;
    size_t index = scene->meshRenderers.size();
    scene->meshRenderers.push_back(meshRenderer);
    scene->meshRendererIndexMap[entityID] = index;
    return &scene->meshRenderers[index];
}

Animator* addAnimator(EntityGroup* scene, uint32_t entityID) {
    Animator animator;
    animator.entityID = entityID;
    size_t index = scene->animators.size();
    scene->animators.push_back(animator);
    scene->animatorIndexMap[entityID] = index;
    return &scene->animators[index];
}

RigidBody* addRigidbody(EntityGroup* scene, uint32_t entityID) {
    RigidBody rigidbody;
    rigidbody.entityID = entityID;
    size_t index = scene->rigidbodies.size();
    scene->rigidbodies.push_back(rigidbody);
    scene->rigidbodyIndexMap[entityID] = index;
    return &scene->rigidbodies[index];
}

PointLight* addPointLight(EntityGroup* scene, uint32_t entityID) {
    PointLight pointLight;
    pointLight.entityID = entityID;
    size_t index = scene->pointLights.size();
    scene->pointLights.push_back(pointLight);
    scene->pointLightIndexMap[entityID] = index;
    return &scene->pointLights[index];
}

SpotLight* addSpotLight(EntityGroup* scene, uint32_t entityID) {
    SpotLight spotLight;
    spotLight.entityID = entityID;
    size_t index = scene->spotLights.size();
    scene->spotLights.push_back(spotLight);
    scene->spotLightIndexMap[entityID] = index;
    return &scene->spotLights[index];
}

Camera* addCamera(EntityGroup* scene, uint32_t entityID) {
    Camera camera;
    camera.entityID = entityID;
    size_t index = scene->cameras.size();
    scene->cameras.push_back(camera);
    scene->cameraIndexMap[entityID] = index;
    return &scene->cameras[index];
}

Player* addPlayer(EntityGroup* scene, uint32_t entityID) {
    Player player;
    player.entityID = entityID;
    size_t index = scene->players.size();
    scene->players.push_back(player);
    scene->playerIndexMap[entityID] = index;
    return &scene->players[index];
}

static void removeTransform(EntityGroup* scene, uint32_t entityID) {
    destroyComponent(scene->transforms, scene->transformIndexMap, entityID);
}

void removeMeshRenderer(EntityGroup* scene, uint32_t entityID) {
    destroyComponent(scene->meshRenderers, scene->meshRendererIndexMap, entityID);
}

void removePlayer(EntityGroup* scene, uint32_t entityID) {
    destroyComponent(scene->players, scene->playerIndexMap, entityID);
}

void removeAnimator(EntityGroup* scene, uint32_t entityID) {
    destroyComponent(scene->animators, scene->animatorIndexMap, entityID);
}

void removeCamera(EntityGroup* scene, uint32_t entityID) {
    destroyComponent(scene->cameras, scene->cameraIndexMap, entityID);
}

void setRigidbodyMoving(EntityGroup* scene, uint32_t entityID) {
    RigidBody* rb = getRigidbody(scene, entityID);
    if (rb != nullptr) {
        if (!scene->movingRigidbodies.count(rb->entityID)) {
            scene->movingRigidbodies.insert(rb->entityID);
        }
    }
}

void setRigidbodyNonMoving(EntityGroup* scene, uint32_t entityID) {
    RigidBody* rb = getRigidbody(scene, entityID);
    if (rb != nullptr) {
        if (scene->movingRigidbodies.count(rb->entityID)) {
            scene->movingRigidbodies.erase(rb->entityID);
        }
    }
}

void removeRigidbody(EntityGroup* scene, uint32_t entityID, JPH::BodyInterface* bodyInterface) {
    RigidBody* rb = getRigidbody(scene, entityID);
    if (rb != nullptr) {
        /*         if (scene->bodyInterface->GetObjectLayer(rb->joltBody) == Layers::MOVING) {
                    scene->movingRigidbodies.erase(rb->entityID);
                } */

        if (scene->movingRigidbodies.count(rb->entityID)) {
            scene->movingRigidbodies.erase(rb->entityID);
        }

        if (bodyInterface != nullptr) {
            bodyInterface->RemoveBody(rb->joltBody);
            bodyInterface->DestroyBody(rb->joltBody);
        }
    }

    destroyComponent(scene->rigidbodies, scene->rigidbodyIndexMap, entityID);
}
void removeSpotLight(EntityGroup* scene, uint32_t entityID) {
    SpotLight* spotLight = getSpotLight(scene, entityID);
    if (spotLight != nullptr && spotLight->enableShadows) {
        deleteSpotLightShadowMap(spotLight);
    }

    destroyComponent(scene->spotLights, scene->spotLightIndexMap, entityID);
}
void removePointLight(EntityGroup* scene, uint32_t entityID) {
    destroyComponent(scene->pointLights, scene->pointLightIndexMap, entityID);
}

void destroyEntity(EntityGroup* entityGroup, uint32_t entityID, JPH::BodyInterface* bodyInterface) {
    if (!entityGroup->entityIndexMap.count(entityID)) {
        return;
    }

    size_t indexToRemove = entityGroup->entityIndexMap[entityID];
    size_t lastIndex = entityGroup->entityIndexMap.size() - 1;

    Transform* transform = getTransform(entityGroup, entityID);
    if (transform->parentEntityID != INVALID_ID) {
        Transform* parent = getTransform(entityGroup, transform->parentEntityID);

        for (int i = 0; i < parent->childEntityIds.size(); i++) {
            if (parent->childEntityIds[i] == entityID) {
                size_t childIndex = i;
                size_t lastIndex = 0;

                if (parent->childEntityIds.size() > 0) {
                    lastIndex = parent->childEntityIds.size() - 1;
                }

                if (childIndex != lastIndex) {
                    std::swap(parent->childEntityIds[childIndex], parent->childEntityIds[lastIndex]);
                }

                parent->childEntityIds.pop_back();
                break;
            }
        }
    }

    std::vector<uint32_t> temp;
    for (uint32_t childID : transform->childEntityIds) {
        temp.push_back(childID);
    }

    for (uint32_t id : temp) {
        destroyEntity(entityGroup, id, bodyInterface);
    }

    removeTransform(entityGroup, entityID);
    removeMeshRenderer(entityGroup, entityID);
    removeAnimator(entityGroup, entityID);
    removeRigidbody(entityGroup, entityID, bodyInterface);
    removePlayer(entityGroup, entityID);
    removeCamera(entityGroup, entityID);
    removeSpotLight(entityGroup, entityID);
    removePointLight(entityGroup, entityID);
    destroyComponent(entityGroup->entities, entityGroup->entityIndexMap, entityID);
}

uint32_t createEntityFromModel(EntityGroup* scene, PhysicsScene* physicsScene, ModelNode* node, uint32_t parentEntityID, bool addColliders, uint32_t rootEntity, bool first, bool isDynamic) {
    uint32_t childEntity = getNewEntity(scene, node->name)->entityID;
    Entity* entity = getEntity(scene, childEntity);

    if (first) {
        rootEntity = childEntity;
    }

    Transform* transform = getTransform(scene, childEntity);
    transform->worldTransform = node->transform;
    setParent(scene, childEntity, parentEntityID);
    entity->name = node->name;

    if (node->mesh != nullptr) {
        MeshRenderer* meshRenderer = addMeshRenderer(scene, childEntity);
        meshRenderer->mesh = node->mesh;
        meshRenderer->rootEntity = rootEntity;

        if (addColliders) {
            JPH::BoxShapeSettings floor_shape_settings(node->mesh->extent);
            // floor_shape_settings.SetEmbedded();  // A ref counted object on the stack (base class RefTarget) should be marked as such to prevent it from being freed when its reference count goes to 0.

            JPH::ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
            JPH::ShapeRefC floor_shape = floor_shape_result.Get();  // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()
            JPH::ObjectLayer layer = isDynamic ? Layers::MOVING : Layers::NON_MOVING;
            JPH::EActivation shouldActivate = isDynamic ? JPH::EActivation::Activate : JPH::EActivation::DontActivate;
            JPH::EMotionType motionType = isDynamic ? JPH::EMotionType::Dynamic : JPH::EMotionType::Static;
            JPH::BodyCreationSettings floor_settings(floor_shape, getPosition(scene, childEntity), getRotation(scene, childEntity), motionType, layer);
            JPH::Body* floor = physicsScene->bodyInterface->CreateBody(floor_settings);
            physicsScene->bodyInterface->AddBody(floor->GetID(), shouldActivate);

            RigidBody* rb = addRigidbody(scene, childEntity);
            rb->lastPosition = getPosition(scene, childEntity);
            rb->lastRotation = getRotation(scene, childEntity);
            rb->joltBody = floor->GetID();
        }
    }

    for (int i = 0; i < node->children.size(); i++) {
        createEntityFromModel(scene, physicsScene, node->children[i], childEntity, addColliders, rootEntity, false, isDynamic);
    }

    return childEntity;
}

static uint32_t copyEntityInternal(Scene* scene, EntityCopier* copier, uint32_t parentID) {
    uint32_t templateID = copier->templateID;
    Entity* templateEntity = getEntity(copier->fromGroup, copier->templateID);
    std::string name = templateEntity->name;
    bool isActive = templateEntity->isActive;
    uint32_t newEntity = getNewEntity(copier->toGroup, name)->entityID;
    copier->relativeIDMap[templateID] = newEntity;
    Transform* templateTransform = getTransform(copier->fromGroup, copier->templateID);
    Transform* newTransform = getTransform(copier->toGroup, newEntity);
    // newTransform->childEntityIds = templateTransform->childEntityIds;
    newTransform->localPosition = templateTransform->localPosition;
    newTransform->localRotation = templateTransform->localRotation;
    newTransform->localScale = templateTransform->localScale;
    newTransform->worldTransform = templateTransform->worldTransform;
    newTransform->parentEntityID = parentID;
    setParent(copier->toGroup, newEntity, parentID);

    for (int i = 0; i < templateTransform->childEntityIds.size(); i++) {
        copier->templateID = templateTransform->childEntityIds[i];
        copyEntityInternal(scene, copier, newEntity);
    }

    MeshRenderer* meshRenderer = getMeshRenderer(copier->fromGroup, templateID);
    Animator* animator = getAnimator(copier->fromGroup, templateID);
    RigidBody* rb = getRigidbody(copier->fromGroup, templateID);
    PointLight* pointLight = getPointLight(copier->fromGroup, templateID);
    SpotLight* spotLight = getSpotLight(copier->fromGroup, templateID);
    Camera* camera = getCamera(copier->fromGroup, templateID);
    Player* player = getPlayer(copier->fromGroup, templateID);

    if (meshRenderer != nullptr) {
        MeshRenderer* newMeshRenderer = addMeshRenderer(copier->toGroup, newEntity);
        newMeshRenderer->boneMatrices = meshRenderer->boneMatrices;
        newMeshRenderer->boneMatricesSet = false;
        newMeshRenderer->materials = meshRenderer->materials;
        newMeshRenderer->mesh = meshRenderer->mesh;
        newMeshRenderer->subMeshes = meshRenderer->subMeshes;
        newMeshRenderer->vao = meshRenderer->vao;
        newMeshRenderer->rootEntity = meshRenderer->rootEntity;
        copier->meshRenderersTemp.push_back(newMeshRenderer->entityID);
        initializeMeshRenderer(copier->toGroup, newMeshRenderer);
    }

    if (animator != nullptr) {
        Animator* newAnimator = addAnimator(copier->toGroup, newEntity);
        newAnimator->animations = animator->animations;
        initializeAnimator(copier->toGroup, newAnimator);
    }

    if (rb != nullptr) {
        RigidBody* newRB = addRigidbody(copier->toGroup, newEntity);
        newRB->shape = rb->shape;
        newRB->mass = rb->mass;
        newRB->halfExtents = rb->halfExtents;
        newRB->halfHeight = rb->halfHeight;
        newRB->layer = rb->layer;
        newRB->motionType = rb->motionType;
        newRB->center = rb->center;
        newRB->radius = rb->radius;
        newRB->rotationLocked = rb->rotationLocked;
        initializeRigidbody(newRB, &scene->physicsScene, copier->toGroup);
    }

    if (pointLight != nullptr) {
        PointLight* newLight = addPointLight(copier->toGroup, newEntity);
        newLight->brightness = pointLight->brightness;
        newLight->color = pointLight->color;
        newLight->isActive = pointLight->isActive;
    }

    if (spotLight != nullptr) {
        SpotLight* newLight = addSpotLight(copier->toGroup, newEntity);
        newLight->blockerSearchUV = spotLight->blockerSearchUV;
        newLight->lightRadiusUV = spotLight->lightRadiusUV;
        newLight->brightness = spotLight->brightness;
        newLight->color = spotLight->color;
        newLight->cutoff = spotLight->cutoff;
        newLight->outerCutoff = spotLight->outerCutoff;
        newLight->enableShadows = spotLight->enableShadows;
        newLight->isActive = spotLight->isActive;
        newLight->range = spotLight->range;
        newLight->shadowWidth = spotLight->shadowWidth;
        newLight->shadowHeight = spotLight->shadowHeight;
        if (newLight->enableShadows) {
            createSpotLightShadowMap(newLight);
        }
    }

    if (camera != nullptr) {
        Camera* newCam = addCamera(copier->toGroup, newEntity);
        newCam->fov = camera->fov;
        newCam->fovRadians = JPH::DegreesToRadians(newCam->fov);
        newCam->nearPlane = camera->nearPlane;
        newCam->farPlane = camera->farPlane;
    }

    if (player != nullptr) {
        Player* newPlayer = addPlayer(copier->toGroup, newEntity);
        CameraController* newCamController = &newPlayer->cameraController;
        newPlayer->armsID = copier->relativeIDMap[player->armsID];
        newPlayer->groundCheckDistance = player->groundCheckDistance;
        newPlayer->jumpHeight = player->jumpHeight;
        newPlayer->moveSpeed = player->moveSpeed;
        newCamController->cameraTargetEntityID = copier->relativeIDMap[player->cameraController.cameraTargetEntityID];
        newCamController->moveSpeed = player->cameraController.moveSpeed;
        newCamController->sensitivity = player->cameraController.sensitivity;
    }

    return newEntity;
}

uint32_t copyEntity(Scene* scene, EntityCopier* copier) {
    uint32_t newID = copyEntityInternal(scene, copier, INVALID_ID);

    for (int i = 0; i < copier->meshRenderersTemp.size(); i++) {
        MeshRenderer* meshRenderer = getMeshRenderer(copier->toGroup, copier->meshRenderersTemp[i]);
        meshRenderer->rootEntity = copier->relativeIDMap[meshRenderer->rootEntity];
    }

    copier->meshRenderersTemp.clear();
    copier->animatorsTemp.clear();
    copier->rigidbodiesTemp.clear();
    copier->pointLightsTemp.clear();
    copier->spotLightsTemp.clear();
    copier->camerasTemp.clear();
    copier->playersTemp.clear();
    copier->relativeIDMap.clear();
    return newID;
}
