#include "ecs.h"
#include "scene.h"
#include "loader.h"

uint32_t getEntityID(Scene* scene) {
    uint32_t newID = scene->nextEntityID++;

    while (scene->entityIndexMap.count(newID)) {
        newID = scene->nextEntityID++;
    }

    scene->entityIndexMap[newID] = 0;
    return newID;
}

Entity* getEntity(Scene* scene, const uint32_t entityID) {
    return &scene->entities[scene->entityIndexMap[entityID]];
}

Transform* getTransform(Scene* scene, const uint32_t entityID) {
    return &scene->transforms[scene->transformIndexMap[entityID]];
}

MeshRenderer* getMeshRenderer(Scene* scene, const uint32_t entityID) {
    if (!scene->meshRendererIndexMap.count(entityID)) {
        return nullptr;
    }

    return &scene->meshRenderers[scene->meshRendererIndexMap[entityID]];
}

RigidBody* getRigidbody(Scene* scene, const uint32_t entityID) {
    if (!scene->rigidbodyIndexMap.count(entityID)) {
        return nullptr;
    }

    return &scene->rigidbodies[scene->rigidbodyIndexMap[entityID]];
}

Animator* getAnimator(Scene* scene, const uint32_t entityID) {
    if (!scene->animatorIndexMap.count(entityID)) {
        return nullptr;
    }

    return &scene->animators[scene->animatorIndexMap[entityID]];
}

PointLight* getPointLight(Scene* scene, const uint32_t entityID) {
    if (!scene->pointLightIndexMap.count(entityID)) {
        return nullptr;
    }

    return &scene->pointLights[scene->pointLightIndexMap[entityID]];
}

SpotLight* getSpotLight(Scene* scene, const uint32_t entityID) {
    if (!scene->spotLightIndexMap.count(entityID)) {
        return nullptr;
    }

    return &scene->spotLights[scene->spotLightIndexMap[entityID]];
}

Camera* getCamera(Scene* scene, const uint32_t entityID) {
    Camera* foundCamera = nullptr;

    for (Camera* camera : scene->cameras) {
        if (camera->entityID == entityID) {
            foundCamera = camera;
            break;
        }
    }

    return foundCamera;
}

Transform* addTransform(Scene* scene, uint32_t entityID) {
    Transform transform;
    transform.entityID = entityID;
    transform.parentEntityID = INVALID_ID;
    size_t index = scene->transforms.size();
    scene->transforms.push_back(transform);
    scene->transformIndexMap[entityID] = index;
    return &scene->transforms[index];
}

Entity* getNewEntity(Scene* scene, std::string name, uint32_t id, bool createTransform) {
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

MeshRenderer* addMeshRenderer(Scene* scene, uint32_t entityID) {
    MeshRenderer meshRenderer;
    meshRenderer.entityID = entityID;
    size_t index = scene->meshRenderers.size();
    scene->meshRenderers.push_back(meshRenderer);
    scene->meshRendererIndexMap[entityID] = index;
    return &scene->meshRenderers[index];
}

RigidBody* addRigidbody(Scene* scene, uint32_t entityID) {
    RigidBody rigidbody;
    rigidbody.entityID = entityID;
    size_t index = scene->rigidbodies.size();
    scene->rigidbodies.push_back(rigidbody);
    scene->rigidbodyIndexMap[entityID] = index;
    return &scene->rigidbodies[index];
}

PointLight* addPointLight(Scene* scene, uint32_t entityID) {
    PointLight pointLight;
    pointLight.entityID = entityID;
    size_t index = scene->pointLights.size();
    scene->pointLights.push_back(pointLight);
    scene->pointLightIndexMap[entityID] = index;
    return &scene->pointLights[index];
}

SpotLight* addSpotLight(Scene* scene, uint32_t entityID) {
    SpotLight spotLight;
    spotLight.entityID = entityID;
    size_t index = scene->spotLights.size();
    scene->spotLights.push_back(spotLight);
    scene->spotLightIndexMap[entityID] = index;
    return &scene->spotLights[index];
}

void destroyEntity(Scene* scene, uint32_t entityID) {
    if (!scene->entityIndexMap.count(entityID)) {
        return;
    }

    size_t indexToRemove = scene->entityIndexMap[entityID];
    size_t lastIndex = scene->entityIndexMap.size() - 1;

    Transform* transform = getTransform(scene, entityID);
    if (transform->parentEntityID != INVALID_ID) {
        Transform* parent = getTransform(scene, transform->parentEntityID);

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

        // parent->childEntityIds.erase(std::remove(parent->childEntityIds.begin(), parent->childEntityIds.end(), entityID), parent->childEntityIds.end());
    }

    std::vector<uint32_t> temp;
    for (uint32_t childID : transform->childEntityIds) {
        temp.push_back(childID);
    }

    for (uint32_t id : temp) {
        destroyEntity(scene, id);
    }

    /* for (size_t i = 0; i < transform->childEntityIds.size(); i++) {
        destroyEntity(scene, transform->childEntityIds[i]);
    } */

    destroyComponent(scene->transforms, scene->transformIndexMap, entityID);
    destroyComponent(scene->meshRenderers, scene->meshRendererIndexMap, entityID);

    RigidBody* rb = getRigidbody(scene, entityID);
    if (rb != nullptr) {
        if (scene->bodyInterface->GetObjectLayer(rb->joltBody) == Layers::MOVING) {
            scene->movingRigidbodies.erase(rb->entityID);
        }
        scene->bodyInterface->RemoveBody(rb->joltBody);
        scene->bodyInterface->DestroyBody(rb->joltBody);
    }

    destroyComponent(scene->rigidbodies, scene->rigidbodyIndexMap, entityID);
    destroyComponent(scene->animators, scene->animatorIndexMap, entityID);
    destroyComponent(scene->spotLights, scene->spotLightIndexMap, entityID);
    destroyComponent(scene->pointLights, scene->pointLightIndexMap, entityID);
    destroyComponent(scene->entities, scene->entityIndexMap, entityID);
}

void mapAnimationChannels(Scene* scene, Animator* animator, uint32_t entityID) {
    size_t entityIndex = scene->entityIndexMap[entityID];
    Entity* entity = &scene->entities[entityIndex];

    size_t transformIndex = scene->transformIndexMap[entityID];
    Transform* transform = &scene->transforms[transformIndex];

    for (Animation* animation : animator->animations) {
        for (AnimationChannel* channel : animation->channels) {
            if (entity->name == channel->name) {
                animator->channelMap[channel] = entity->entityID;
            }
        }
    }

    for (int i = 0; i < transform->childEntityIds.size(); i++) {
        mapAnimationChannels(scene, animator, transform->childEntityIds[i]);
    }
}

Animator* addAnimator(Scene* scene, uint32_t entityID, Model* model) {
    Animator animator;
    animator.entityID = entityID;
    size_t index = scene->animators.size();
    scene->animators.push_back(animator);
    scene->animatorIndexMap[entityID] = index;
    Animator* animatorPtr = &scene->animators[index];

    for (Animation* animation : model->animations) {
        animatorPtr->animations.push_back(animation);
    }

    for (Animation* animation : animatorPtr->animations) {
        animatorPtr->animationMap[animation->name] = animation;
    }

    mapAnimationChannels(scene, animatorPtr, entityID);

    animatorPtr->currentAnimation = animatorPtr->animations[0];
    return animatorPtr;
}

Animator* addAnimator(Scene* scene, uint32_t entityID, std::vector<Animation*> animations) {
    Animator animator;
    animator.entityID = entityID;
    animator.animations = animations;

    for (Animation* animation : animator.animations) {
        animator.animationMap[animation->name] = animation;
    }

    size_t index = scene->animators.size();
    scene->animators.push_back(animator);
    scene->animatorIndexMap[entityID] = index;
    Animator* animatorPtr = &scene->animators[index];

    mapAnimationChannels(scene, animatorPtr, entityID);
    animatorPtr->currentAnimation = animatorPtr->animations[0];
    return animatorPtr;
}

Camera* addCamera(Scene* scene, uint32_t entityID, float fov, float nearPlane, float farPlane) {
    Camera* camera = new Camera();
    scene->cameras.push_back(camera);
    camera->entityID = entityID;
    camera->fov = fov;
    camera->fovRadians = JPH::DegreesToRadians(fov);
    camera->aspectRatio = 16.0f / 9.0f;
    camera->nearPlane = nearPlane;
    camera->farPlane = farPlane;
    return camera;
}

uint32_t createEntityFromModel(Scene* scene, ModelNode* node, uint32_t parentEntityID, bool addColliders, uint32_t rootEntity, bool first, bool isDynamic) {
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
            JPH::Body* floor = scene->bodyInterface->CreateBody(floor_settings);
            scene->bodyInterface->AddBody(floor->GetID(), shouldActivate);

            RigidBody* rb = addRigidbody(scene, childEntity);
            rb->lastPosition = getPosition(scene, childEntity);
            rb->lastRotation = getRotation(scene, childEntity);
            rb->joltBody = floor->GetID();
        }
    }

    for (int i = 0; i < node->children.size(); i++) {
        createEntityFromModel(scene, node->children[i], childEntity, addColliders, rootEntity, false, isDynamic);
    }

    return childEntity;
}