#include "physics.h"
#include "transform.h"

void updateRigidBodies(Scene* scene) {
    float deltaTime = scene->deltaTime;

    for (RigidBody rigidbody : scene->rigidbodies) {
        rigidbody.linearVelocity.y += scene->gravity * deltaTime;
        rigidbody.linearMagnitude = glm::length(rigidbody.linearVelocity);
        glm::vec3 newPosition = getPosition(scene, rigidbody.entityID) + rigidbody.linearVelocity * deltaTime;
        setPosition(scene, rigidbody.entityID, newPosition);
    }

    glm::vec3 collisionResolution = glm::vec3(0.0f);
    float totalDamping = 0.0f;

    for (int i = 0; i < scene->rigidbodies.size(); i++) {
        RigidBody* rigidbodyA = &scene->rigidbodies[i];

        size_t colliderAIndex = scene->colliderIndices[rigidbodyA->entityID];
        BoxCollider* colliderA = &scene->boxColliders[colliderAIndex];
        if (!colliderA->isActive) {
            continue;
        }

        totalDamping = rigidbodyA->linearDrag;
        for (int j = i + 1; j < scene->rigidbodies.size(); j++) {
            RigidBody* rigidbodyB = &scene->rigidbodies[j];

            size_t colliderBIndex = scene->colliderIndices[rigidbodyB->entityID];
            BoxCollider* colliderB = &scene->boxColliders[colliderBIndex];

            if (!colliderB->isActive) {
                continue;
            }

            if (checkAABB(scene, *colliderA, *colliderB, collisionResolution)) {
                size_t entityIndex = scene->entityIndices[rigidbodyA->entityID];
                Entity* entityA = &scene->entities[entityIndex];

                size_t entityIndexB = scene->entityIndices[rigidbodyB->entityID];
                Entity* entityB = &scene->entities[entityIndexB];

                std::cout << "collision between: " << entityA->name << " and " << entityB->name << std::endl;
                setPosition(scene, rigidbodyA->entityID, getPosition(scene, rigidbodyA->entityID) + (collisionResolution / 2.0f));
                setPosition(scene, rigidbodyB->entityID, getPosition(scene, rigidbodyB->entityID) - (collisionResolution / 2.0f));

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

        for (BoxCollider collider : scene->boxColliders) {
            if (collider.entityID == colliderA->entityID || !collider.isActive) {
                continue;
            }

            if (checkAABB(scene, *colliderA, collider, collisionResolution)) {
                size_t entityIndex = scene->entityIndices[colliderA->entityID];
                Entity* entityA = &scene->entities[entityIndex];

                size_t entityIndexB = scene->entityIndices[collider.entityID];
                Entity* entityB = &scene->entities[entityIndexB];

                std::cout << "collision between: " << entityA->name << " and " << entityB->name << std::endl;

                setPosition(scene, rigidbodyA->entityID, getPosition(scene, rigidbodyA->entityID) + collisionResolution);

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

bool checkAABB(Scene* scene, BoxCollider& colliderA, BoxCollider& colliderB, glm::vec3& resolutionOut) {
    glm::vec3 centerA = getPosition(scene, colliderA.entityID) + colliderA.center;
    glm::vec3 centerB = getPosition(scene, colliderB.entityID) + colliderB.center;
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