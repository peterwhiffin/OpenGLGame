#include "transform.h"
#include "ecs.h"

void updateTransformMatrices(EntityGroup* scene, Transform* transform) {
    mat4 worldTransform;
    uint32_t parentID;
    Transform* childTransform;

    mat4 translation = mat4::sTranslation(transform->localPosition);
    transform->localRotation = transform->localRotation.Normalized();
    mat4 rotation = mat4::sRotation(transform->localRotation);
    mat4 scale = mat4::sScale(transform->localScale);
    worldTransform = translation * rotation * scale;
    parentID = transform->parentEntityID;

    if (parentID != INVALID_ID) {
        worldTransform = getTransform(scene, parentID)->worldTransform * worldTransform;
    }

    transform->worldTransform = worldTransform;

    for (uint32_t childID : transform->childEntityIds) {
        childTransform = getTransform(scene, childID);
        updateTransformMatrices(scene, childTransform);
    }
}

vec3 QuaternionByVector3(quat rotation, vec3 point) {
    return rotation.Normalized() * point.Normalized();  // who knows
}

vec3 transformRight(EntityGroup* scene, uint32_t entityID) {
    return QuaternionByVector3(getRotation(scene, entityID), vec3(1.0f, 0.0f, 0.0f));
}

vec3 transformUp(EntityGroup* scene, uint32_t entityID) {
    return QuaternionByVector3(getRotation(scene, entityID), vec3(0.0f, 1.0f, 0.0f));
}

vec3 transformForward(EntityGroup* scene, uint32_t entityID) {
    return QuaternionByVector3(getRotation(scene, entityID), vec3(0.0f, 0.0f, 1.0f));
}

vec3 positionFromMatrix(mat4& matrix) {
    return matrix.GetTranslation();
}

quat quatFromMatrix(mat4& matrix) {
    return matrix.GetQuaternion();
}

vec3 scaleFromMatrix(mat4& matrix) {
    vec3 scale;
    scale.SetX(matrix.GetAxisX().Length());
    scale.SetY(matrix.GetAxisY().Length());
    scale.SetZ(matrix.GetAxisZ().Length());
    return scale;
}

vec3 getPosition(EntityGroup* scene, uint32_t entityID) {
    Transform* transform = getTransform(scene, entityID);
    return transform->worldTransform.GetTranslation();
}

quat getRotation(EntityGroup* scene, uint32_t entityID) {
    Transform* transform = getTransform(scene, entityID);
    return quatFromMatrix(transform->worldTransform);
}

vec3 getScale(EntityGroup* scene, uint32_t entityID) {
    Transform* transform = getTransform(scene, entityID);
    return scaleFromMatrix(transform->worldTransform);
}

vec3 getLocalPosition(EntityGroup* scene, uint32_t entityID) {
    Transform* transform = getTransform(scene, entityID);
    return transform->localPosition;
}

quat getLocalRotation(EntityGroup* scene, uint32_t entityID) {
    Transform* transform = getTransform(scene, entityID);
    return transform->localRotation;
}

vec3 getLocalScale(EntityGroup* scene, uint32_t entityID) {
    Transform* transform = getTransform(scene, entityID);
    return transform->localScale;
}

void setPosition(EntityGroup* scene, uint32_t entityID, vec3 position) {
    Transform* transform = getTransform(scene, entityID);
    uint32_t parentEntityID = transform->parentEntityID;

    if (parentEntityID == INVALID_ID) {
        transform->localPosition = position;
    } else {
        Transform* parent = getTransform(scene, parentEntityID);
        transform->localPosition = vec3(parent->worldTransform.Inversed() * position);  // may be wrong. maybe vec4(position, 1.0)
    }

    updateTransformMatrices(scene, transform);
}

void setRotation(EntityGroup* scene, uint32_t entityID, quat rotation) {
    Transform* transform = getTransform(scene, entityID);
    uint32_t parentEntityID = transform->parentEntityID;

    if (parentEntityID == INVALID_ID) {
        transform->localRotation = rotation;
    } else {
        Transform* parent = getTransform(scene, parentEntityID);
        quat parentRotation = quatFromMatrix(parent->worldTransform);
        transform->localRotation = parentRotation.Inversed() * rotation;
    }

    updateTransformMatrices(scene, transform);
}

void setScale(EntityGroup* scene, uint32_t entityID, vec3 scale) {
    Transform* transform = getTransform(scene, entityID);
    uint32_t parentEntityID = transform->parentEntityID;

    if (parentEntityID == INVALID_ID) {
        transform->localScale = scale;
    } else {
        transform->localScale = scale / getScale(scene, parentEntityID);
    }

    updateTransformMatrices(scene, transform);
}

void setLocalPosition(EntityGroup* scene, uint32_t entityID, vec3 localPosition) {
    Transform* transform = getTransform(scene, entityID);
    transform->localPosition = localPosition;
    updateTransformMatrices(scene, transform);
}

void setLocalRotation(EntityGroup* scene, uint32_t entityID, quat localRotation) {
    Transform* transform = getTransform(scene, entityID);
    transform->localRotation = localRotation;
    updateTransformMatrices(scene, transform);
}

void setLocalScale(EntityGroup* scene, uint32_t entityID, vec3 localScale) {
    Transform* transform = getTransform(scene, entityID);
    transform->localScale = localScale;
    updateTransformMatrices(scene, transform);
}

void removeParent(EntityGroup* scene, uint32_t entityID) {
    Transform* transform = getTransform(scene, entityID);
    uint32_t parentEntityID = transform->parentEntityID;

    if (parentEntityID == INVALID_ID) {
        return;
    }

    Transform* parent = getTransform(scene, parentEntityID);
    std::vector<uint32_t>& childEntityIds = parent->childEntityIds;

    transform->localPosition = getPosition(scene, entityID);
    transform->localRotation = getRotation(scene, entityID);
    transform->localScale = getScale(scene, entityID);
    childEntityIds.erase(std::remove(childEntityIds.begin(), childEntityIds.end(), entityID), childEntityIds.end());
    transform->parentEntityID = INVALID_ID;
    updateTransformMatrices(scene, transform);
}

void setParent(EntityGroup* scene, uint32_t childEntityID, uint32_t parentEntityID) {
    removeParent(scene, childEntityID);
    Transform* child = getTransform(scene, childEntityID);

    if (parentEntityID != INVALID_ID) {
        Transform* parent = getTransform(scene, parentEntityID);
        mat4 parentWorldToLocalMatrix = parent->worldTransform.Inversed() * child->worldTransform;

        child->localPosition = positionFromMatrix(parentWorldToLocalMatrix);
        child->localRotation = quatFromMatrix(parentWorldToLocalMatrix);
        child->localScale = scaleFromMatrix(parentWorldToLocalMatrix);

        parent->childEntityIds.push_back(child->entityID);
        child->parentEntityID = parent->entityID;
        updateTransformMatrices(scene, parent);
    }
}
