#include "component.h"
#include "transform.h"

uint32_t getEntityID(Scene* scene) {
    uint32_t newID = scene->nextEntityID++;

    while (scene->usedIds.count(newID) != 0) {
        newID = scene->nextEntityID++;
    }

    scene->usedIds[newID] = 0;
    return newID;
}

void registerEntityID(Scene* scene, uint32_t id) {
    scene->usedIds[id] = 0;
}

Entity* getEntity(Scene* scene, uint32_t entityID) {
    return &scene->entities[scene->entityIndexMap[entityID]];
}

Transform* getTransform(Scene* scene, uint32_t entityID) {
    return &scene->transforms[scene->transformIndexMap[entityID]];
}

MeshRenderer* getMeshRenderer(Scene* scene, uint32_t entityID) {
    return &scene->meshRenderers[scene->meshRendererIndexMap[entityID]];
}

BoxCollider* getBoxCollider(Scene* scene, uint32_t entityID) {
    if (scene->boxColliderIndexMap.count(entityID) == 0) {
        return nullptr;
    }

    return &scene->boxColliders[scene->boxColliderIndexMap[entityID]];
}

RigidBody* getRigidbody(Scene* scene, uint32_t entityID) {
    return &scene->rigidbodies[scene->rigidbodyIndexMap[entityID]];
}

Animator* getAnimator(Scene* scene, uint32_t entityID) {
    return &scene->animators[scene->animatorIndexMap[entityID]];
}

PointLight* getPointLight(Scene* scene, uint32_t entityID) {
    if (scene->pointLightIndexMap.count(entityID) == 0) {
        return nullptr;
    }

    return &scene->pointLights[scene->pointLightIndexMap[entityID]];
}

Camera* getCamera(Scene* scene, uint32_t entityID) {
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
    size_t index = scene->transforms.size();
    scene->transforms.push_back(transform);
    scene->transformIndexMap[entityID] = index;
    return &scene->transforms[index];
}

Entity* getNewEntity(Scene* scene, std::string name, uint32_t id, bool createTransform) {
    Entity entity;
    if (id == -1) {
        entity.id = getEntityID(scene);
    } else {
        registerEntityID(scene, id);
        entity.id = id;
    }

    entity.name = name;
    size_t index = scene->entities.size();
    scene->entities.push_back(entity);
    scene->entityIndexMap[entity.id] = index;
    if (createTransform) {
        addTransform(scene, entity.id);
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

BoxCollider* addBoxCollider(Scene* scene, uint32_t entityID) {
    BoxCollider boxCollider;
    boxCollider.entityID = entityID;
    size_t index = scene->boxColliders.size();
    scene->boxColliders.push_back(boxCollider);
    scene->boxColliderIndexMap[entityID] = index;
    return &scene->boxColliders[index];
}

RigidBody* addRigidbody(Scene* scene, uint32_t entityID) {
    RigidBody rigidbody;
    rigidbody.entityID = entityID;
    rigidbody.linearVelocity = glm::vec3(0.0f);
    rigidbody.linearMagnitude = 0.0f;
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

void mapAnimationChannels(Scene* scene, Animator* animator, uint32_t entityID) {
    size_t entityIndex = scene->entityIndexMap[entityID];
    Entity* entity = &scene->entities[entityIndex];

    size_t transformIndex = scene->transformIndexMap[entityID];
    Transform* transform = &scene->transforms[transformIndex];

    for (Animation* animation : animator->animations) {
        for (AnimationChannel* channel : animation->channels) {
            if (entity->name == channel->name) {
                animator->channelMap[channel] = entity->id;
                animator->nextKeyPosition[channel] = 0;
                animator->nextKeyRotation[channel] = 0;
                animator->nextKeyScale[channel] = 0;
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

    mapAnimationChannels(scene, animatorPtr, entityID);

    animatorPtr->currentAnimation = animatorPtr->animations[0];
    return animatorPtr;
}

Animator* addAnimator(Scene* scene, uint32_t entityID, std::vector<Animation*> animations) {
    Animator animator;
    animator.entityID = entityID;
    size_t index = scene->animators.size();
    scene->animators.push_back(animator);
    scene->animatorIndexMap[entityID] = index;
    Animator* animatorPtr = &scene->animators[index];
    animator.animations = animations;
    /*
        for (Animation* animation : del->animations) {
            animatorPtr->animations.push_back(animation);
        } */

    mapAnimationChannels(scene, animatorPtr, entityID);

    animatorPtr->currentAnimation = animatorPtr->animations[0];
    return animatorPtr;
}

Camera* addCamera(Scene* scene, uint32_t entityID, float fov, float aspectRatio, float nearPlane, float farPlane) {
    Camera* camera = new Camera();
    scene->cameras.push_back(camera);
    camera->entityID = entityID;
    camera->fov = glm::radians(fov);
    camera->aspectRatio = aspectRatio;
    camera->nearPlane = nearPlane;
    camera->farPlane = farPlane;
    return camera;
}

uint32_t createEntityFromModel(Scene* scene, ModelNode* node, uint32_t parentEntityID, bool addColliders) {
    uint32_t childEntity = getNewEntity(scene, node->name)->id;
    Entity* entity = getEntity(scene, childEntity);
    setParent(scene, childEntity, parentEntityID);
    entity->name = node->name;

    if (node->mesh != nullptr) {
        MeshRenderer* meshRenderer = addMeshRenderer(scene, childEntity);
        meshRenderer->mesh = node->mesh;

        if (addColliders) {
            BoxCollider* boxCollider = addBoxCollider(scene, childEntity);
            boxCollider->center = node->mesh->center;
            boxCollider->extent = node->mesh->extent;
            boxCollider->isActive = true;
        }
    }

    for (int i = 0; i < node->children.size(); i++) {
        createEntityFromModel(scene, node->children[i], childEntity, addColliders);
    }

    return childEntity;
}