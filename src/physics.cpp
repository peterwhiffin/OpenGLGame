#include "physics.h"
#include "transform.h"

void updateRigidBodies(std::vector<RigidBody>& rigidbodies, std::vector<BoxCollider>& colliders, float gravity, float deltaTime) {
    for (RigidBody rigidbody : rigidbodies) {
        rigidbody.linearVelocity.y += gravity * deltaTime;
        rigidbody.linearMagnitude = glm::length(rigidbody.linearVelocity);
        glm::vec3 newPosition = getPosition(rigidbody.transform) + rigidbody.linearVelocity * deltaTime;
        setPosition(rigidbody.transform, newPosition);
    }

    glm::vec3 collisionResolution = glm::vec3(0.0f);
    float totalDamping = 0.0f;

    for (int i = 0; i < rigidbodies.size(); i++) {
        RigidBody* rigidbodyA = &rigidbodies[i];
        if (!rigidbodyA->collider->isActive) {
            continue;
        }

        totalDamping = rigidbodyA->linearDrag;
        for (int j = i + 1; j < rigidbodies.size(); j++) {
            RigidBody* rigidbodyB = &rigidbodies[j];
            if (!rigidbodyB->collider->isActive) {
                continue;
            }
            if (checkAABB(*rigidbodyA->collider, *rigidbodyB->collider, collisionResolution)) {
                setPosition(rigidbodyA->transform, getPosition(rigidbodyA->transform) + (collisionResolution / 2.0f));
                setPosition(rigidbodyB->transform, getPosition(rigidbodyB->transform) - (collisionResolution / 2.0f));

                glm::vec3 flatForce = glm::normalize(rigidbodyA->linearVelocity - rigidbodyB->linearVelocity);
                flatForce.y = 0.0f;

                if (collisionResolution.y != 0.0f) {
                    rigidbodyA->linearVelocity.y = -2.0f;
                    rigidbodyB->linearVelocity.y = -2.0f;
                    flatForce *= 0.1f;
                } else {
                    // totalDamping += rigidbodyA->friction;
                }
                float velocityDiff = glm::abs(rigidbodyB->linearMagnitude - rigidbodyA->linearMagnitude) * 0.8f;
                float rigidBodyAForce = 1.0f - rigidbodyA->mass / (rigidbodyA->mass + rigidbodyB->mass);
                float rigidbodyBForce = 1.0f - rigidBodyAForce;
                rigidbodyA->linearVelocity -= flatForce * rigidBodyAForce * velocityDiff;
                rigidbodyB->linearVelocity += flatForce * rigidbodyBForce * velocityDiff;
            }
        }

        for (BoxCollider collider : colliders) {
            if (&collider == rigidbodyA->collider || !collider.isActive) {
                continue;
            }

            if (checkAABB(*rigidbodyA->collider, collider, collisionResolution)) {
                setPosition(rigidbodyA->transform, getPosition(rigidbodyA->transform) + collisionResolution);

                if (collisionResolution.y != 0.0f) {
                    rigidbodyA->linearVelocity.y = -2.0f;
                    totalDamping += rigidbodyA->friction;
                }
            }
        }

        applyDamping(*rigidbodyA, totalDamping, deltaTime);
    }
}

void applyDamping(RigidBody& rigidbody, float damping, float deltaTime) {
    float epsilon = 1e-6f;
    rigidbody.linearMagnitude = glm::length(rigidbody.linearVelocity);

    if (rigidbody.linearMagnitude > epsilon) {
        glm::vec3 normalVel = glm::normalize(rigidbody.linearVelocity);
        rigidbody.linearMagnitude = glm::max(rigidbody.linearMagnitude - (damping * deltaTime), 0.0f);
        rigidbody.linearVelocity = normalVel * rigidbody.linearMagnitude;
    } else {
        rigidbody.linearVelocity = glm::vec3(0.0f);
    }
}

bool checkAABB(BoxCollider& colliderA, BoxCollider& colliderB, glm::vec3& resolutionOut) {
    glm::vec3 centerA = getPosition(colliderA.transform) + colliderA.center;
    glm::vec3 centerB = getPosition(colliderB.transform) + colliderB.center;
    glm::vec3 delta = centerA - centerB;
    glm::vec3 overlap = colliderA.extent + colliderB.extent - glm::abs(delta);

    resolutionOut = glm::vec3(0.0f);

    if (overlap.x <= 0.0f || overlap.y <= 0.0f || overlap.z <= 0.0f) {
        return false;
    }

    float minOverlap = overlap.x;
    float push = delta.x < 0 ? -1.0f : 1.0f;
    glm::vec3 pushDir = glm::vec3(push, 0.0f, 0.0f);

    if (overlap.y < minOverlap) {
        minOverlap = overlap.y;
        push = delta.y < 0 ? -1.0f : 1.0f;
        pushDir = glm::vec3(0.0f, push, 0.0f);
    }

    if (overlap.z < minOverlap) {
        minOverlap = overlap.z;
        push = delta.z < 0 ? -1.0f : 1.0f;
        pushDir = glm::vec3(0.0f, 0.0f, push);
    }

    resolutionOut = pushDir * minOverlap;
    return true;
}