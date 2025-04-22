#include "transform.h"

void updateTransformMatrices(Scene* scene, Transform* transform) {
    transform->worldTransform = glm::translate(glm::mat4(1.0f), transform->localPosition);
    transform->worldTransform *= glm::mat4_cast(transform->localRotation);
    transform->worldTransform = glm::scale(transform->worldTransform, transform->localScale);

    if (transform->parentEntityID != INVALID_ID) {
        size_t index = scene->transformIndices[transform->parentEntityID];
        Transform* parentTransform = &scene->transforms[index];

        transform->worldTransform = parentTransform->worldTransform * transform->worldTransform;
    }

    for (uint32_t entityID : transform->childEntityIds) {
        size_t index = scene->transformIndices[entityID];
        Transform* child = &scene->transforms[index];
        updateTransformMatrices(scene, child);
    }
}

glm::vec3 QuaternionByVector3(glm::quat rotation, glm::vec3 point) {
    float num = rotation.x + rotation.x;
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

    return result;
}

glm::vec3 right(Scene* scene, uint32_t entityID) {
    size_t index = scene->transformIndices[entityID];
    Transform* transform = &scene->transforms[index];
    return QuaternionByVector3(transform->localRotation, glm::vec3(1.0f, 0.0f, 0.0f));
}

glm::vec3 up(Scene* scene, uint32_t entityID) {
    size_t index = scene->transformIndices[entityID];
    Transform* transform = &scene->transforms[index];
    return QuaternionByVector3(transform->localRotation, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::vec3 forward(Scene* scene, uint32_t entityID) {
    size_t index = scene->transformIndices[entityID];
    Transform* transform = &scene->transforms[index];
    return QuaternionByVector3(transform->localRotation, glm::vec3(0.0f, 0.0f, -1.0f));
}

glm::vec3 positionFromMatrix(glm::mat4& matrix) {
    return matrix[3];
}

glm::quat quatFromMatrix(glm::mat4& matrix) {
    glm::vec3 scale;
    scale.x = glm::length(glm::vec3(matrix[0]));
    scale.y = glm::length(glm::vec3(matrix[1]));
    scale.z = glm::length(glm::vec3(matrix[2]));

    glm::mat3 rotationMatrix;
    rotationMatrix[0] = glm::vec3(matrix[0]) / scale.x;
    rotationMatrix[1] = glm::vec3(matrix[1]) / scale.y;
    rotationMatrix[2] = glm::vec3(matrix[2]) / scale.z;

    return glm::quat_cast(rotationMatrix);
}

glm::vec3 scaleFromMatrix(glm::mat4& matrix) {
    glm::vec3 scale;
    scale.x = glm::length(glm::vec3(matrix[0]));
    scale.y = glm::length(glm::vec3(matrix[1]));
    scale.z = glm::length(glm::vec3(matrix[2]));
    return scale;
}

glm::vec3 getPosition(Scene* scene, uint32_t entityID) {
    size_t index = scene->transformIndices[entityID];
    Transform* transform = &scene->transforms[index];
    return transform->worldTransform[3];
}

glm::quat getRotation(Scene* scene, uint32_t entityID) {
    size_t index = scene->transformIndices[entityID];
    Transform* transform = &scene->transforms[index];
    return quatFromMatrix(transform->worldTransform);
}

glm::vec3 getScale(Scene* scene, uint32_t entityID) {
    size_t index = scene->transformIndices[entityID];
    Transform* transform = &scene->transforms[index];
    glm::vec3 scale;
    scale.x = glm::length(glm::vec3(transform->worldTransform[0]));
    scale.y = glm::length(glm::vec3(transform->worldTransform[1]));
    scale.z = glm::length(glm::vec3(transform->worldTransform[2]));
    return scale;
}

glm::vec3 getLocalPosition(Scene* scene, uint32_t entityID) {
    size_t index = scene->transformIndices[entityID];
    Transform* transform = &scene->transforms[index];
    return transform->localPosition;
}

glm::quat getLocalRotation(Scene* scene, uint32_t entityID) {
    size_t index = scene->transformIndices[entityID];
    Transform* transform = &scene->transforms[index];
    return transform->localRotation;
}

glm::vec3 getLocalScale(Scene* scene, uint32_t entityID) {
    size_t index = scene->transformIndices[entityID];
    Transform* transform = &scene->transforms[index];
    return transform->localScale;
}

void setPosition(Scene* scene, uint32_t entityID, glm::vec3 position) {
    size_t index = scene->transformIndices[entityID];
    Transform* transform = &scene->transforms[index];

    if (transform->parentEntityID == INVALID_ID) {
        transform->localPosition = position;
    } else {
        size_t index = scene->transformIndices[transform->parentEntityID];
        Transform* parent = &scene->transforms[index];
        transform->localPosition = glm::vec3(glm::inverse(parent->worldTransform) * glm::vec4(position, 1.0f));
    }

    updateTransformMatrices(scene, transform);
}

void setRotation(Scene* scene, uint32_t entityID, glm::quat rotation) {
    size_t index = scene->transformIndices[entityID];
    Transform* transform = &scene->transforms[index];

    if (transform->parentEntityID == INVALID_ID) {
        transform->localRotation = rotation;
    } else {
        size_t index = scene->transformIndices[transform->parentEntityID];
        Transform* parent = &scene->transforms[index];
        glm::quat parentRotation = quatFromMatrix(parent->worldTransform);
        transform->localRotation = glm::inverse(parentRotation) * rotation;
    }

    updateTransformMatrices(scene, transform);
}

void setScale(Scene* scene, uint32_t entityID, glm::vec3 scale) {
    size_t index = scene->transformIndices[entityID];
    Transform* transform = &scene->transforms[index];

    if (transform->parentEntityID == INVALID_ID) {
        transform->localScale = scale;
    } else {
        transform->localScale = scale / getScale(scene, transform->parentEntityID);
    }

    updateTransformMatrices(scene, transform);
}

void setLocalPosition(Scene* scene, uint32_t entityID, glm::vec3 localPosition) {
    size_t index = scene->transformIndices[entityID];
    Transform* transform = &scene->transforms[index];

    transform->localPosition = localPosition;
    updateTransformMatrices(scene, transform);
}

void setLocalRotation(Scene* scene, uint32_t entityID, glm::quat localRotation) {
    size_t index = scene->transformIndices[entityID];
    Transform* transform = &scene->transforms[index];

    transform->localRotation = localRotation;
    updateTransformMatrices(scene, transform);
}

void setLocalScale(Scene* scene, uint32_t entityID, glm::vec3 localScale) {
    size_t index = scene->transformIndices[entityID];
    Transform* transform = &scene->transforms[index];

    transform->localScale = localScale;
    updateTransformMatrices(scene, transform);
}

void removeParent(Scene* scene, uint32_t entityID) {
    size_t index = scene->transformIndices[entityID];
    Transform* transform = &scene->transforms[index];

    if (transform->parentEntityID == INVALID_ID) {
        return;
    }

    transform->localPosition = getPosition(scene, entityID);
    transform->localRotation = getRotation(scene, entityID);
    transform->localScale = getScale(scene, entityID);

    size_t parentIndex = scene->transformIndices[transform->parentEntityID];
    Transform* parent = &scene->transforms[parentIndex];

    parent->childEntityIds.erase(
        std::remove(parent->childEntityIds.begin(), parent->childEntityIds.end(), entityID),
        parent->childEntityIds.end());

    transform->parentEntityID = INVALID_ID;
    updateTransformMatrices(scene, transform);
}

void setParent(Scene* scene, uint32_t childEntityID, uint32_t parentEntityID) {
    size_t childIndex = scene->transformIndices[childEntityID];
    Transform* child = &scene->transforms[childIndex];
    removeParent(scene, childEntityID);

    if (parentEntityID != INVALID_ID) {
        size_t parentIndex = scene->transformIndices[parentEntityID];
        Transform* parent = &scene->transforms[parentIndex];
        glm::mat4 parentWorldToLocalMatrix = glm::inverse(parent->worldTransform) * child->worldTransform;

        child->localPosition = positionFromMatrix(parentWorldToLocalMatrix);
        child->localRotation = quatFromMatrix(parentWorldToLocalMatrix);
        child->localScale = scaleFromMatrix(parentWorldToLocalMatrix);

        parent->childEntityIds.push_back(child->entityID);
        child->parentEntityID = parent->entityID;
        updateTransformMatrices(scene, parent);
    }
}