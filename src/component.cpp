#include "component.h"
#include "transform.h"
#include "shader.h"

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
    if (scene->meshRendererIndexMap.count(entityID) == 0) {
        return nullptr;
    }

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
    if (scene->animatorIndexMap.count(entityID) == 0) {
        return nullptr;
    }

    return &scene->animators[scene->animatorIndexMap[entityID]];
}

PointLight* getPointLight(Scene* scene, uint32_t entityID) {
    if (scene->pointLightIndexMap.count(entityID) == 0) {
        return nullptr;
    }

    return &scene->pointLights[scene->pointLightIndexMap[entityID]];
}

SpotLight* getSpotLight(Scene* scene, uint32_t entityID) {
    if (scene->spotLightIndexMap.count(entityID) == 0) {
        return nullptr;
    }

    return &scene->spotLights[scene->spotLightIndexMap[entityID]];
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
        entity.entityID = getEntityID(scene);
    } else {
        registerEntityID(scene, id);
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

void findBones(Scene* scene, MeshRenderer* renderer, Transform* parent) {
    for (int i = 0; i < parent->childEntityIds.size(); i++) {
        Entity* child = getEntity(scene, parent->childEntityIds[i]);
        if (renderer->mesh->boneNameMap.count(child->name)) {
            renderer->transformBoneMap[child->entityID] = renderer->mesh->boneNameMap[child->name];
        }

        findBones(scene, renderer, getTransform(scene, child->entityID));
    }
}

void mapBones(Scene* scene, MeshRenderer* renderer) {
    if (renderer->mesh->boneNameMap.size() == 0) {
        return;
    }

    renderer->boneMatrices.reserve(100);

    for (int i = 0; i < 100; i++) {
        renderer->boneMatrices.push_back(glm::mat4(1.0f));
    }

    Transform* parent = getTransform(scene, renderer->entityID);

    if (parent->parentEntityID != INVALID_ID) {
        parent = getTransform(scene, parent->parentEntityID);
    }

    findBones(scene, renderer, parent);
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

SpotLight* addSpotLight(Scene* scene, uint32_t entityID) {
    SpotLight spotLight;
    spotLight.entityID = entityID;
    size_t index = scene->spotLights.size();
    scene->spotLights.push_back(spotLight);
    scene->spotLightIndexMap[entityID] = index;
    return &scene->spotLights[index];
}

void destroyEntity(Scene* scene, uint32_t entityID) {
    size_t indexToRemove = scene->entityIndexMap[entityID];
    size_t lastIndex = scene->entityIndexMap.size() - 1;

    Transform* transform = getTransform(scene, entityID);
    if (transform->parentEntityID != INVALID_ID) {
        Transform* parent = getTransform(scene, transform->parentEntityID);
        parent->childEntityIds.erase(std::remove(parent->childEntityIds.begin(), parent->childEntityIds.end(), entityID), parent->childEntityIds.end());
    }

    for (size_t i = 0; i < transform->childEntityIds.size(); i++) {
        destroyEntity(scene, transform->childEntityIds[i]);
    }

    destroyComponent(scene->transforms, scene->transformIndexMap, entityID);
    destroyComponent(scene->meshRenderers, scene->meshRendererIndexMap, entityID);
    destroyComponent(scene->boxColliders, scene->boxColliderIndexMap, entityID);
    destroyComponent(scene->rigidbodies, scene->rigidbodyIndexMap, entityID);
    destroyComponent(scene->animators, scene->animatorIndexMap, entityID);

    if (destroyComponent(scene->spotLights, scene->spotLightIndexMap, entityID)) {
        glUseProgram(scene->lightingShader);
        glUniform1i(6, scene->spotLights.size());
    }

    if (destroyComponent(scene->pointLights, scene->pointLightIndexMap, entityID)) {
        glUseProgram(scene->lightingShader);
        glUniform1i(7, scene->pointLights.size());
    }

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

Camera* addCamera(Scene* scene, uint32_t entityID, float fov, float aspectRatio, float nearPlane, float farPlane) {
    Camera* camera = new Camera();
    scene->cameras.push_back(camera);
    camera->entityID = entityID;
    camera->fov = fov;
    camera->fovRadians = glm::radians(fov);
    camera->aspectRatio = (float)scene->windowData.viewportWidth / scene->windowData.viewportHeight;
    camera->nearPlane = nearPlane;
    camera->farPlane = farPlane;
    return camera;
}

uint32_t createEntityFromModel(Scene* scene, ModelNode* node, uint32_t parentEntityID, bool addColliders, uint32_t rootEntity, bool first) {
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
            BoxCollider* boxCollider = addBoxCollider(scene, childEntity);
            boxCollider->center = node->mesh->center;
            boxCollider->extent = node->mesh->extent;
            boxCollider->isActive = true;
        }
    }

    for (int i = 0; i < node->children.size(); i++) {
        createEntityFromModel(scene, node->children[i], childEntity, addColliders, rootEntity, false);
    }

    return childEntity;
}