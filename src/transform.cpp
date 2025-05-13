#include "transform.h"

void updateTransformMatrices(Scene* scene, Transform* transform) {
    mat4 worldTransform;
    uint32_t parentID;
    Transform* childTransform;

    /* worldTransform = glm::translate(glm::mat4(1.0f), transform->localPosition);
    worldTransform *= glm::mat4_cast(transform->localRotation);
    worldTransform = glm::scale(worldTransform, transform->localScale);
 */

    mat4 translation = mat4::sTranslation(transform->localPosition);
    mat4 rotation = mat4::sRotation(transform->localRotation);
    mat4 scale = mat4::sScale(transform->localScale);

    worldTransform = translation * rotation * scale;

    // Final transform: T * R * S, like in GLM
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
    /* float num = rotation.x + rotation.x;
    float num2 = rotation.y + rotation.y;
    float num3 = rotation.z + rotation.z;

    float num4 = rotation.x * num;
    float num5 = rotation.y * num2;
    float num6 = rotation.z * num3;

    float num7 = rotation.x * num2;
    float num8 = rotation.x * num3;
    float num9 = rotation.y * num3;

    float num10 = rotation.w * num;
    float num11 = rotation.w * num2;
    float num12 = rotation.w * num3;

    glm::vec3 result = glm::vec3(0.0f, 0.0f, 0.0f);

    result.x = (1.0f - (num5 + num6)) * point.x + (num7 - num12) * point.y + (num8 + num11) * point.z;
    result.y = (num7 + num12) * point.x + (1.0f - (num4 + num6)) * point.y + (num9 - num10) * point.z;
    result.z = (num8 - num11) * point.x + (num9 + num10) * point.y + (1.0f - (num4 + num5)) * point.z;

    return result; */
}

vec3 right(Scene* scene, uint32_t entityID) {
    return QuaternionByVector3(getRotation(scene, entityID), vec3(1.0f, 0.0f, 0.0f));
}

vec3 up(Scene* scene, uint32_t entityID) {
    return QuaternionByVector3(getRotation(scene, entityID), vec3(0.0f, 1.0f, 0.0f));
}

vec3 forward(Scene* scene, uint32_t entityID) {
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

vec3 getPosition(Scene* scene, uint32_t entityID) {
    Transform* transform = getTransform(scene, entityID);
    return transform->worldTransform.GetTranslation();
}

quat getRotation(Scene* scene, uint32_t entityID) {
    Transform* transform = getTransform(scene, entityID);
    return quatFromMatrix(transform->worldTransform);
}

vec3 getScale(Scene* scene, uint32_t entityID) {
    Transform* transform = getTransform(scene, entityID);
    return scaleFromMatrix(transform->worldTransform);
}

vec3 getLocalPosition(Scene* scene, uint32_t entityID) {
    Transform* transform = getTransform(scene, entityID);
    return transform->localPosition;
}

quat getLocalRotation(Scene* scene, uint32_t entityID) {
    Transform* transform = getTransform(scene, entityID);
    return transform->localRotation;
}

vec3 getLocalScale(Scene* scene, uint32_t entityID) {
    Transform* transform = getTransform(scene, entityID);
    return transform->localScale;
}

void setPosition(Scene* scene, uint32_t entityID, vec3 position) {
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

void setRotation(Scene* scene, uint32_t entityID, quat rotation) {
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

void setScale(Scene* scene, uint32_t entityID, vec3 scale) {
    Transform* transform = getTransform(scene, entityID);
    uint32_t parentEntityID = transform->parentEntityID;

    if (parentEntityID == INVALID_ID) {
        transform->localScale = scale;
    } else {
        transform->localScale = scale / getScale(scene, parentEntityID);
    }

    updateTransformMatrices(scene, transform);
}

void setLocalPosition(Scene* scene, uint32_t entityID, vec3 localPosition) {
    Transform* transform = getTransform(scene, entityID);
    transform->localPosition = localPosition;
    updateTransformMatrices(scene, transform);
}

void setLocalRotation(Scene* scene, uint32_t entityID, quat localRotation) {
    Transform* transform = getTransform(scene, entityID);
    transform->localRotation = localRotation;
    updateTransformMatrices(scene, transform);
}

void setLocalScale(Scene* scene, uint32_t entityID, vec3 localScale) {
    Transform* transform = getTransform(scene, entityID);
    transform->localScale = localScale;
    updateTransformMatrices(scene, transform);
}

void removeParent(Scene* scene, uint32_t entityID) {
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

void setParent(Scene* scene, uint32_t childEntityID, uint32_t parentEntityID) {
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