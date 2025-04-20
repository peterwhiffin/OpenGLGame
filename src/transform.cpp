#include <glm/ext/quaternion_float.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "transform.h"

void updateTransformMatrices(Transform* transform) {
    transform->localToWorldMatrix = glm::translate(glm::mat4(1.0f), transform->position);
    transform->localToWorldMatrix *= glm::mat4_cast(transform->rotation);
    transform->localToWorldMatrix = glm::scale(transform->localToWorldMatrix, transform->scale);

    if (transform->parent != nullptr) {
        transform->localToWorldMatrix = transform->parent->localToWorldMatrix * transform->localToWorldMatrix;
    }

    for (Transform* child : transform->children) {
        updateTransformMatrices(child);
    }
}

glm::vec3 getPosition(Transform* transform) {
    return transform->localToWorldMatrix[3];
}

glm::quat getRotation(Transform* transform) {
    return quatFromMatrix(transform->localToWorldMatrix);
}

glm::vec3 getScale(Transform* transform) {
    glm::vec3 scale;
    scale.x = glm::length(glm::vec3(transform->localToWorldMatrix[0]));
    scale.y = glm::length(glm::vec3(transform->localToWorldMatrix[1]));
    scale.z = glm::length(glm::vec3(transform->localToWorldMatrix[2]));
    return scale;
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

void setLocalPosition(Transform* transform, glm::vec3 localPosition) {
    transform->position = localPosition;
    updateTransformMatrices(transform);
}
void setLocalRotation(Transform* transform, glm::quat localRotation) {
    transform->rotation = localRotation;
    updateTransformMatrices(transform);
}
void setLocalScale(Transform* transform, glm::vec3 localScale) {
    transform->scale = localScale;
    updateTransformMatrices(transform);
}
void setPosition(Transform* transform, glm::vec3 position) {
    if (transform->parent == nullptr) {
        transform->position = position;
    } else {
        transform->position = glm::vec3(glm::inverse(transform->parent->localToWorldMatrix) * glm::vec4(position, 1.0f));
    }

    updateTransformMatrices(transform);
}

void setRotation(Transform* transform, glm::quat rotation) {
    if (transform->parent == nullptr) {
        transform->rotation = rotation;
    } else {
        glm::quat parentRotation = quatFromMatrix(transform->parent->localToWorldMatrix);
        transform->rotation = glm::inverse(parentRotation) * rotation;
    }

    updateTransformMatrices(transform);
}

void setScale(Transform* transform, glm::vec3 scale) {
    if (transform->parent == nullptr) {
        transform->scale = scale;
    } else {
        transform->scale = scale / getScale(transform->parent);
    }

    updateTransformMatrices(transform);
}

glm::vec3 right(Transform* transform) {
    return QuaternionByVector3(transform->rotation, glm::vec3(1.0f, 0.0f, 0.0f));
}

glm::vec3 up(Transform* transform) {
    return QuaternionByVector3(transform->rotation, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::vec3 forward(Transform* transform) {
    return QuaternionByVector3(transform->rotation, glm::vec3(0.0f, 0.0f, -1.0f));
}

void setParent(Transform* child, Transform* parent) {
    removeParent(child);

    if (parent != nullptr) {
        glm::mat4 parentWorldToLocalMatrix = glm::inverse(parent->localToWorldMatrix) * child->localToWorldMatrix;

        child->position = positionFromMatrix(parentWorldToLocalMatrix);
        child->rotation = quatFromMatrix(parentWorldToLocalMatrix);
        child->scale = scaleFromMatrix(parentWorldToLocalMatrix);

        parent->children.push_back(child);
        child->parent = parent;
        updateTransformMatrices(parent);
    }
}

void removeParent(Transform* transform) {
    if (transform->parent == nullptr) {
        return;
    }

    transform->position = getPosition(transform);
    transform->rotation = getRotation(transform);
    transform->scale = getScale(transform);

    transform->parent->children.erase(
        std::remove(transform->parent->children.begin(), transform->parent->children.end(), transform),
        transform->parent->children.end());

    transform->parent = nullptr;
    updateTransformMatrices(transform);
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