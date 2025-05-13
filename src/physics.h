#pragma once
#include "component.h"

namespace Layers {
static constexpr JPH::ObjectLayer NON_MOVING = 0;
static constexpr JPH::ObjectLayer MOVING = 1;
static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
};  // namespace Layers

namespace BroadPhaseLayers {
static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
static constexpr JPH::BroadPhaseLayer MOVING(1);
static constexpr JPH::uint NUM_LAYERS(2);
};  // namespace BroadPhaseLayers

void applyDamping(RigidBody* rigidbody, float damping, float deltaTime);

void updateRigidBodies(Scene* scene);
bool checkAABB(Scene* scene, BoxCollider* colliderA, BoxCollider* colliderB, vec3* resolutionOut);