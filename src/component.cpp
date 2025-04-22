#include "component.h"
#include "transform.h"

uint32_t getEntityID(Scene* scene) {
    scene->nextEntityID += 1;
    return scene->nextEntityID;
}

Transform* addTransform(Scene* scene, uint32_t entityID) {
    Transform transform;
    transform.entityID = entityID;
    size_t index = scene->transforms.size();
    scene->transforms.push_back(transform);
    scene->transformIndices[entityID] = index;
    return &scene->transforms[index];
}

Entity* getNewEntity(Scene* scene, std::string name) {
    Entity entity;
    entity.id = getEntityID(scene);
    entity.name = name;
    size_t index = scene->entities.size();
    scene->entities.push_back(entity);
    addTransform(scene, entity.id);
    return &scene->entities[index];
}

MeshRenderer* addMeshRenderer(Scene* scene, uint32_t entityID) {
    MeshRenderer meshRenderer;
    meshRenderer.entityID = entityID;
    size_t index = scene->renderers.size();
    scene->renderers.push_back(meshRenderer);
    scene->rendererIndices[entityID] = index;
    return &scene->renderers[index];
}

BoxCollider* addBoxCollider(Scene* scene, uint32_t entityID) {
    BoxCollider boxCollider;
    boxCollider.entityID = entityID;
    size_t index = scene->boxColliders.size();
    scene->boxColliders.push_back(boxCollider);
    scene->colliderIndices[entityID] = index;
    return &scene->boxColliders[index];
}

RigidBody* addRigidbody(Scene* scene, uint32_t entityID) {
    RigidBody rigidbody;
    rigidbody.entityID = entityID;
    size_t index = scene->rigidbodies.size();
    scene->rigidbodies.push_back(rigidbody);
    scene->rigidbodyIndices[entityID] = index;
    return &scene->rigidbodies[index];
}

void mapAnimationChannels(Scene* scene, Animator* animator, uint32_t entityID) {
    size_t entityIndex = scene->entityIndices[entityID];
    Entity* entity = &scene->entities[entityIndex];

    size_t transformIndex = scene->transformIndices[entityID];
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
    scene->animatorIndices[entityID] = index;
    Animator* animatorPtr = &scene->animators[index];

    for (Animation* animation : model->animations) {
        animator.animations.push_back(animation);
    }

    mapAnimationChannels(scene, animatorPtr, entityID);

    return animatorPtr;
}

Entity* createEntityFromModel(Scene* scene, Model* model, ModelNode* node, uint32_t parentEntityID, bool first, bool addColliders) {
    Entity* childEntity = getNewEntity(scene, node->name);
    childEntity->name = node->name;

    if (parentEntityID != INVALID_ID) {
        setParent(scene, childEntity->id, parentEntityID);
    }

    if (node->mesh != nullptr) {
        MeshRenderer* meshRenderer = addMeshRenderer(scene, childEntity->id);
        meshRenderer->mesh = node->mesh;

        if (addColliders) {
            BoxCollider* boxCollider = addBoxCollider(scene, childEntity->id);
            boxCollider->center = node->mesh->center;
            boxCollider->extent = node->mesh->extent;
            boxCollider->isActive = true;
        }
    }

    for (int i = 0; i < node->children.size(); i++) {
        createEntityFromModel(scene, model, node->children[i], childEntity->id, false, addColliders);
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