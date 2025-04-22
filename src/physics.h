#pragma once
#include "component.h"

void applyDamping(RigidBody& rigidbody, float damping, float deltaTime);

void updateRigidBodies(Scene* scene);
bool checkAABB(Scene* scene, BoxCollider& colliderA, BoxCollider& colliderB, glm::vec3& resolutionOut);