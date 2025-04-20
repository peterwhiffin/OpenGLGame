#pragma once
#include <vector>
#include "component.h"
struct RigidBody;
struct BoxCollider;

void updateRigidBodies(std::vector<RigidBody*>& rigidbodies, std::vector<BoxCollider*>& colliders, float gravity, float deltaTime);
void applyDamping(RigidBody& rigidbody, float damping, float deltaTime);
bool checkAABB(BoxCollider& colliderA, BoxCollider& colliderB, glm::vec3& resolutionOut);
