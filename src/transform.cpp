#include "transform.h"

void updateTransformMatrices(Transform* transform) {
    transform->worldTransform = glm::translate(glm::mat4(1.0f), transform->localPosition);
    transform->worldTransform *= glm::mat4_cast(transform->localRotation);
    transform->worldTransform = glm::scale(transform->worldTransform, transform->localScale);

    if (transform->parent != nullptr) {
        transform->worldTransform = transform->parent->worldTransform * transform->worldTransform;
    }

    for (Transform* child : transform->children) {
        updateTransformMatrices(child);
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

glm::vec3 right(Transform* transform) {
    return QuaternionByVector3(transform->localRotation, glm::vec3(1.0f, 0.0f, 0.0f));
}

glm::vec3 up(Transform* transform) {
    return QuaternionByVector3(transform->localRotation, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::vec3 forward(Transform* transform) {
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

glm::vec3 getPosition(Transform* transform) {
    return transform->worldTransform[3];
}

glm::quat getRotation(Transform* transform) {
    return quatFromMatrix(transform->worldTransform);
}

glm::vec3 getScale(Transform* transform) {
    glm::vec3 scale;
    scale.x = glm::length(glm::vec3(transform->worldTransform[0]));
    scale.y = glm::length(glm::vec3(transform->worldTransform[1]));
    scale.z = glm::length(glm::vec3(transform->worldTransform[2]));
    return scale;
}

glm::vec3 getLocalPosition(Transform* transform) {
    return transform->localPosition;
}

glm::quat getLocalRotation(Transform* transform) {
    return transform->localRotation;
}

glm::vec3 getLocalScale(Transform* transform) {
    return transform->localScale;
}

void setPosition(Transform* transform, glm::vec3 position) {
    if (transform->parent == nullptr) {
        transform->localPosition = position;
    } else {
        transform->localPosition = glm::vec3(glm::inverse(transform->parent->worldTransform) * glm::vec4(position, 1.0f));
    }

    updateTransformMatrices(transform);
}

void setRotation(Transform* transform, glm::quat rotation) {
    if (transform->parent == nullptr) {
        transform->localRotation = rotation;
    } else {
        glm::quat parentRotation = quatFromMatrix(transform->parent->worldTransform);
        transform->localRotation = glm::inverse(parentRotation) * rotation;
    }

    updateTransformMatrices(transform);
}

void setScale(Transform* transform, glm::vec3 scale) {
    if (transform->parent == nullptr) {
        transform->localScale = scale;
    } else {
        transform->localScale = scale / getScale(transform->parent);
    }

    updateTransformMatrices(transform);
}

void setLocalPosition(Transform* transform, glm::vec3 localPosition) {
    transform->localPosition = localPosition;
    updateTransformMatrices(transform);
}

void setLocalRotation(Transform* transform, glm::quat localRotation) {
    transform->localRotation = localRotation;
    updateTransformMatrices(transform);
}

void setLocalScale(Transform* transform, glm::vec3 localScale) {
    transform->localScale = localScale;
    updateTransformMatrices(transform);
}

void removeParent(Scene* scene, uint32_t index) {
    if (transform->parent == nullptr) {
        return;
    }

    transform->localPosition = getPosition(transform);
    transform->localRotation = getRotation(transform);
    transform->localScale = getScale(transform);

    transform->parent->children.erase(
        std::remove(transform->parent->children.begin(), transform->parent->children.end(), transform),
        transform->parent->children.end());

    transform->parent = nullptr;
    updateTransformMatrices(transform);
}

void setParent(Scene* scene, uint32_t childTransformIndex, uint32_t parentTransformIndex) {
    removeParent(scene, childTransformIndex);

    if (parentTransformIndex != INVALID_INDEX) {
        Transform* child = &scene->transforms[childTransformIndex];
        Transform* parent = &scene->transforms[parentTransformIndex];
        glm::mat4 parentWorldToLocalMatrix = glm::inverse(parent->worldTransform) * child->worldTransform;

        child->localPosition = positionFromMatrix(parentWorldToLocalMatrix);
        child->localRotation = quatFromMatrix(parentWorldToLocalMatrix);
        child->localScale = scaleFromMatrix(parentWorldToLocalMatrix);

        parent->childEntityIds.push_back(child->entityID);
        child->parentEntityID = parent->entityID;
        updateTransformMatrices(parent);
    }
}