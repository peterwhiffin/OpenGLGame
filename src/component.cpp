#include "component.h"
#include "transform.h"

unsigned int getEntityID(unsigned int& nextEntityID) {
    unsigned int id = nextEntityID;
    nextEntityID++;
    return id;
}

Entity* createEntityFromModel(Model* model, ModelNode* node, std::vector<MeshRenderer*>* renderers, Entity* parentEntity, bool first, unsigned int nextEntityID) {
    Entity* childEntity = new Entity();
    childEntity->name = node->name;
    childEntity->transform.entity = childEntity;
    childEntity->transform.parent = nullptr;
    Transform* parentTransform = nullptr;
    if (parentEntity != nullptr) {
        parentTransform = &parentEntity->transform;
    }
    setParent(&childEntity->transform, parentTransform);

    /*     if (first && addAnimator) {
            animator = new Animator(childEntity);
            for (Animation* animation : model->animations) {
                animator->animations.push_back(animation);
            }

            animator->currentAnimation = animator->animations[0];

            animators->push_back(animator);
        }
     */
    if (node->mesh != nullptr) {
        MeshRenderer* meshRenderer = new MeshRenderer();
        meshRenderer->entity = childEntity;
        meshRenderer->mesh = node->mesh;
        meshRenderer->transform = &childEntity->transform;
        renderers->push_back(meshRenderer);

        /*       if (addCollider) {
                  BoxCollider* collider = new BoxCollider(childEntity);
                  collider->center = meshRenderer->mesh->center;
                  collider->extent = meshRenderer->mesh->extent;
                  childEntity->components.push_back(collider);
                  colliders->push_back(collider);
              }

              if (addAnimator) {
                  if (model->channelMap.count(node) != 0) {
                      animator->channelMap[model->channelMap[node]] = &childEntity->transform;

                      std::cout << "channel mapped: " << animator->channelMap[model->channelMap[node]]->entity->name << std::endl;
                      animator->nextKeyPosition[model->channelMap[node]] = 0;
                      animator->nextKeyRotation[model->channelMap[node]] = 0;
                      animator->nextKeyScale[model->channelMap[node]] = 0;
                  }
              } */

        childEntity->id = getEntityID(nextEntityID);
        childEntity->name = node->name;
    }

    for (int i = 0; i < node->children.size(); i++) {
        createEntityFromModel(model, node->children[i], renderers, childEntity, false, nextEntityID);
    }

    return childEntity;
}