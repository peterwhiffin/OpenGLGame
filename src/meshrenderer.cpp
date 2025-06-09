#include "meshrenderer.h"
#include "scene.h"

static void findBones(EntityGroup* entities, MeshRenderer* renderer, Transform* parent) {
    for (int i = 0; i < parent->childEntityIds.size(); i++) {
        Entity* child = getEntity(entities, parent->childEntityIds[i]);
        if (renderer->mesh->boneNameMap.count(child->name)) {
            renderer->transformBoneMap[child->entityID] = renderer->mesh->boneNameMap[child->name];
        }

        findBones(entities, renderer, getTransform(entities, child->entityID));
    }
}

static void mapBones(EntityGroup* entities, MeshRenderer* renderer) {
    if (renderer->mesh->boneNameMap.size() == 0) {
        return;
    }

    renderer->boneMatrices.clear();
    renderer->boneMatrices.reserve(100);

    for (int i = 0; i < 100; i++) {
        renderer->boneMatrices.push_back(mat4::sIdentity());
    }

    Transform* parent = getTransform(entities, renderer->entityID);
    if (parent->parentEntityID != INVALID_ID) {
        parent = getTransform(entities, parent->parentEntityID);
    }

    // renderer->rootEntity = parent->entityID;
    findBones(entities, renderer, parent);
}

void initializeMeshRenderer(EntityGroup* entities, MeshRenderer* meshRenderer) {
    mapBones(entities, meshRenderer);
}
