#include "component.h"
#include "transform.h"

unsigned int getEntityID(unsigned int* nextEntityID) {
    unsigned int id = *nextEntityID;
    *nextEntityID++;
    return id;
}

Entity* getNewEntity(unsigned int* nextEntityID) {
    Entity* entity = new Entity();
    entity->id = getEntityID(nextEntityID);
    entity->components[component::kTransform] = &entity->transform;
    return entity;
}

Entity* createEntityFromModel(Model* model, ModelNode* node, std::vector<MeshRenderer*>* renderers, Entity* parentEntity, std::vector<BoxCollider*>* colliders, bool first, bool addColliders, unsigned int* nextEntityID) {
    Entity* childEntity = getNewEntity(nextEntityID);
    childEntity->name = node->name;
    childEntity->transform.entity = childEntity;
    childEntity->transform.parent = nullptr;
    childEntity->name = node->name;
    Transform* parentTransform = nullptr;

    if (parentEntity != nullptr) {
        parentTransform = &parentEntity->transform;
    }

    setParent(&childEntity->transform, parentTransform);

    if (node->mesh != nullptr) {
        MeshRenderer* meshRenderer = new MeshRenderer();
        meshRenderer->entity = childEntity;
        meshRenderer->mesh = node->mesh;
        meshRenderer->transform = &childEntity->transform;
        renderers->push_back(meshRenderer);
        childEntity->components[component::kMeshRenderer] = meshRenderer;

        if (addColliders) {
            addBoxCollider(childEntity, node->mesh->center, node->mesh->extent, colliders);
        }
    }

    for (int i = 0; i < node->children.size(); i++) {
        createEntityFromModel(model, node->children[i], renderers, childEntity, colliders, false, addColliders, nextEntityID);
    }

    return childEntity;
}

void mapAnimationChannels(Animator* animator, Entity* entity) {
    for (Animation* animation : animator->animations) {
        for (AnimationChannel* channel : animation->channels) {
            if (entity->name == channel->name) {
                animator->channelMap[channel] = &entity->transform;
                animator->nextKeyPosition[channel] = 0;
                animator->nextKeyRotation[channel] = 0;
                animator->nextKeyScale[channel] = 0;
            }
        }
    }

    for (int i = 0; i < entity->transform.children.size(); i++) {
        mapAnimationChannels(animator, entity->transform.children[i]->entity);
    }
}

Animator* addAnimator(Entity* entity, Model* model, std::vector<Animator*>* animators) {
    Animator* animator = new Animator();
    for (Animation* animation : model->animations) {
        animator->animations.push_back(animation);
    }

    mapAnimationChannels(animator, entity);

    animator->currentAnimation = animator->animations[0];
    animators->push_back(animator);
    entity->components[component::kAnimator] = animator;
    return animator;
}

BoxCollider* addBoxCollider(Entity* entity, glm::vec3 center, glm::vec3 halfExtents, std::vector<BoxCollider*>* colliders) {
    BoxCollider* boxCollider = new BoxCollider();
    boxCollider->entity = entity;
    boxCollider->transform = &entity->transform;
    boxCollider->center = center;
    boxCollider->extent = halfExtents;
    boxCollider->isActive = true;
    colliders->push_back(boxCollider);
    entity->components[component::kBoxCollider] = boxCollider;
    return boxCollider;
}

RigidBody* addRigidBody(Entity* entity, float mass, float linearDrag, float friction, std::vector<RigidBody*>* rigidbodies) {
    RigidBody* rigidbody = nullptr;

    if (entity->components.count(component::kBoxCollider) != 0) {
        rigidbody = new RigidBody();
        rigidbody->collider = (BoxCollider*)entity->components[component::kBoxCollider];
        rigidbody->entity = entity;
        rigidbody->transform = &entity->transform;
        rigidbody->mass = mass;
        rigidbody->linearDrag = linearDrag;
        rigidbody->friction = friction;
        rigidbodies->push_back(rigidbody);
        entity->components[component::kRigidBody] = rigidbody;
    }

    return rigidbody;
}

Camera* addCamera(Entity* entity, float fov, float aspectRatio, float nearPlane, float farPlane, std::vector<Camera*>* cameras) {
    Camera* camera = new Camera();
    camera->entity = entity;
    camera->transform = &entity->transform;
    camera->fov = fov;
    camera->aspectRatio = aspectRatio;
    camera->nearPlane = nearPlane;
    camera->farPlane = farPlane;
    cameras->push_back(camera);
    return camera;
}