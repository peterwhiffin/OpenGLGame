#include "physics.h"
#include "transform.h"

void updateRigidBodies(Scene* scene) {
    for (RigidBody& rigidbody : scene->rigidbodies) {
        vec3 newPos = lerp(getPosition(scene, rigidbody.entityID), scene->bodyInterface->GetPosition(rigidbody.joltBody), 0.75f);
        quat newRot = getRotation(scene, rigidbody.entityID).SLERP(scene->bodyInterface->GetRotation(rigidbody.joltBody), 0.75f);
        setPosition(scene, rigidbody.entityID, newPos);
        setRotation(scene, rigidbody.entityID, newRot);
    }

    return;

    /*     for (int i = 0; i < scene->rigidbodies.size(); i++) {
            RigidBody* rigidbodyA = &scene->rigidbodies[i];
            if (!rigidbodyA->joltBody.IsInvalid()) {
                continue;
            }

            BoxCollider* colliderA = getBoxCollider(scene, rigidbodyA->entityID);
            totalDamping = rigidbodyA->linearDrag;

            for (int j = i + 1; j < scene->rigidbodies.size(); j++) {
                RigidBody* rigidbodyB = &scene->rigidbodies[j];

                if (!rigidbodyA->joltBody.IsInvalid()) {
                    continue;
                }

                BoxCollider* colliderB = getBoxCollider(scene, rigidbodyB->entityID);

                if (checkAABB(scene, colliderA, colliderB, &collisionResolution)) {
                    setPosition(scene, rigidbodyA->entityID, getPosition(scene, rigidbodyA->entityID) + (collisionResolution * 0.5f));
                    setPosition(scene, rigidbodyB->entityID, getPosition(scene, rigidbodyB->entityID) - (collisionResolution * 0.5f));

                    vec3 flatForce = rigidbodyA->linearVelocity - rigidbodyB->linearVelocity;

                    if (flatForce.Length() > epsilon) {
                        flatForce = flatForce.Normalized();
                    }

                    flatForce.SetY(0.0f);

                    if (collisionResolution.GetY() != 0.0f) {
                        rigidbodyA->linearVelocity.SetY(-2.0f);
                        rigidbodyB->linearVelocity.SetY(-2.0f);
                        flatForce *= 0.1f;
                    } else {
                        totalDamping += rigidbodyA->friction;
                    }

                    float velocityDiff = JPH::abs(rigidbodyB->linearMagnitude - rigidbodyA->linearMagnitude) * 0.8f;
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

                if (checkAABB(scene, colliderA, &collider, &collisionResolution)) {
                    setPosition(scene, colliderA->entityID, getPosition(scene, colliderA->entityID) + collisionResolution);

                    if (collisionResolution.GetY() != 0.0f) {
                        rigidbodyA->linearVelocity.SetY(-2.0f);
                        totalDamping += rigidbodyA->friction;
                    }
                }
            }

            applyDamping(rigidbodyA, totalDamping, deltaTime);
        } */
}

void applyDamping(RigidBody* rigidbody, float damping, float deltaTime) {
    float epsilon = 1e-6f;
    rigidbody->linearMagnitude = rigidbody->linearVelocity.Length();

    if (rigidbody->linearMagnitude > epsilon) {
        vec3 normalVel = rigidbody->linearVelocity.Normalized();
        rigidbody->linearMagnitude = std::max(rigidbody->linearMagnitude - (damping * deltaTime), 0.0f);
        rigidbody->linearVelocity = normalVel * rigidbody->linearMagnitude;
    } else {
        rigidbody->linearVelocity = vec3(0.0f, 0.0f, 0.0f);
    }
}

bool checkAABB(Scene* scene, BoxCollider* colliderA, BoxCollider* colliderB, vec3* resolutionOut) {
    Transform* transformA = getTransform(scene, colliderA->entityID);
    Transform* transformB = getTransform(scene, colliderB->entityID);
    Entity* entityA = getEntity(scene, colliderA->entityID);
    Entity* entityB = getEntity(scene, colliderB->entityID);
    vec3 centerA = getPosition(scene, colliderA->entityID) + colliderA->center;
    vec3 centerB = getPosition(scene, colliderB->entityID) + colliderB->center;
    vec3 delta = centerA - centerB;
    vec3 overlap = colliderA->extent + colliderB->extent - delta.Abs();

    if (overlap.GetX() <= 0.0f || overlap.GetY() <= 0.0f || overlap.GetZ() <= 0.0f) {
        return false;
    }

    float minOverlap = overlap.GetX();
    float push = delta.GetX() < 0 ? -1.0f : 1.0f;
    vec3 pushDir = vec3(push, 0.0f, 0.0f);

    if (overlap.GetY() < minOverlap) {
        minOverlap = overlap.GetY();
        push = delta.GetY() < 0 ? -1.0f : 1.0f;
        pushDir = vec3(0.0f, push, 0.0f);
    }

    if (overlap.GetZ() < minOverlap) {
        minOverlap = overlap.GetZ();
        push = delta.GetZ() < 0 ? -1.0f : 1.0f;
        pushDir = vec3(0.0f, 0.0f, push);
    }

    *resolutionOut = pushDir * minOverlap;
    return true;
}