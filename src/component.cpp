#include "component.h"
#include "transform.h"

uint32_t getEntityID(Scene* scene) {
    scene->nextEntityID += 1;
    return scene->nextEntityID;
}

bool getEntity(Scene* scene, uint32_t entityID, Entity** entityOut) {
    auto it = scene->entityIndexMap.find(entityID);
    if (it != scene->entityIndexMap.end()) {
        *entityOut = &scene->entities[it->second];
        return true;
    }

    return false;
}

bool getTransform(Scene* scene, uint32_t entityID, Transform** transformOut) {
    auto it = scene->transformIndexMap.find(entityID);
    if (it != scene->transformIndexMap.end()) {
        *transformOut = &scene->transforms[it->second];
        return true;
    }

    return false;
}

bool getMeshRenderer(Scene* scene, uint32_t entityID, MeshRenderer** meshRendererOut) {
    auto it = scene->meshRendererIndexMap.find(entityID);
    if (it != scene->meshRendererIndexMap.end()) {
        *meshRendererOut = &scene->meshRenderers[it->second];
        return true;
    }

    return false;
}

bool getBoxCollider(Scene* scene, uint32_t entityID, BoxCollider** boxColliderOut) {
    auto it = scene->boxColliderIndexMap.find(entityID);
    if (it != scene->boxColliderIndexMap.end()) {
        *boxColliderOut = &scene->boxColliders[it->second];
        return true;
    }

    return false;
}

bool getRigidbody(Scene* scene, uint32_t entityID, RigidBody** rigidbodyOut) {
    auto it = scene->rigidbodyIndexMap.find(entityID);
    if (it != scene->rigidbodyIndexMap.end()) {
        *rigidbodyOut = &scene->rigidbodies[it->second];
        return true;
    }

    return false;
}

bool getAnimator(Scene* scene, uint32_t entityID, Animator** animatorOut) {
    auto it = scene->animatorIndexMap.find(entityID);
    if (it != scene->animatorIndexMap.end()) {
        *animatorOut = &scene->animators[it->second];
        return true;
    }

    return false;
}

Transform* addTransform(Scene* scene, uint32_t entityID) {
    Transform transform;
    transform.entityID = entityID;
    size_t index = scene->transforms.size();
    scene->transforms.push_back(transform);
    scene->transformIndexMap[entityID] = index;
    return &scene->transforms[index];
}

Entity* getNewEntity(Scene* scene, std::string name) {
    Entity entity;
    entity.id = getEntityID(scene);
    entity.name = name;
    size_t index = scene->entities.size();
    scene->entities.push_back(entity);
    scene->entityIndexMap[entity.id] = index;
    addTransform(scene, entity.id);
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
    size_t index = scene->rigidbodies.size();
    scene->rigidbodies.push_back(rigidbody);
    scene->rigidbodyIndexMap[entityID] = index;
    return &scene->rigidbodies[index];
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

uint32_t createEntityFromModel(Scene* scene, ModelNode* node, uint32_t parentEntityID, bool addColliders) {
    uint32_t childEntity = getNewEntity(scene, node->name)->id;
    Entity* entity = nullptr;
    getEntity(scene, childEntity, &entity);
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