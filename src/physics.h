#pragma once
#include <Jolt/Jolt.h>
#include <Jolt/Math/Float2.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Renderer/DebugRendererSimple.h>

#include "utils/mathutils.h"
#include "forward.h"

JPH_SUPPRESS_WARNINGS

struct PhysicsScene {
    JPH::PhysicsSystem* physicsSystem;
    JPH::BodyInterface* bodyInterface;
    JPH::TempAllocatorImpl* tempAllocator;
    JPH::JobSystemThreadPool* jobSystem;
    JPH::BroadPhaseLayerInterface* broad_phase_layer_interface;
    JPH::ObjectVsBroadPhaseLayerFilter* object_vs_broadphase_layer_filter;
    JPH::ObjectLayerPairFilter* object_vs_object_layer_filter;
};

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

const JPH::uint cMaxBodies = 65536;
const JPH::uint cNumBodyMutexes = 0;
const JPH::uint cMaxBodyPairs = 65536;
const JPH::uint cMaxContactConstraints = 10240;
const JPH::uint cCollisionSteps = 1;
constexpr double cDeltaTime = 1.0 / 60.0;

struct RigidBody {
    uint32_t entityID;
    JPH::Color color;
    JPH::BodyID joltBody;
    float mass = 1.0f;
    float radius = 0.5f;
    float halfHeight = 0.5f;
    vec3 center = vec3(0.0f, 0.0f, 0.0f);
    vec3 halfExtents = vec3(0.5f, 0.5f, 0.5f);
    JPH::EShapeSubType shape = JPH::EShapeSubType::Box;
    JPH::EMotionType motionType = JPH::EMotionType::Static;
    JPH::ObjectLayer layer = Layers::NON_MOVING;
    vec3 lastPosition = vec3(0.0f, 0.0f, 0.0f);
    quat lastRotation = quat(0.0f, 0.0f, 0.0f, 1.0f);
    bool rotationLocked = false;
};

void initPhysics(Scene* scene);
void updatePhysics(Scene* scene);
void destroyPhysicsSystem();
void updatePhysicsBodyPositions(Scene* scene);

class MyObjectLayerPairFilter : public JPH::ObjectLayerPairFilter {
   public:
    virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override;
};

class MyBroadPhaseLayerInterface final : public JPH::BroadPhaseLayerInterface {
   public:
    MyBroadPhaseLayerInterface();
    virtual JPH::uint GetNumBroadPhaseLayers() const override;
    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override;

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override;
#endif  // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

   private:
    JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

/// Class that determines if an object layer can collide with a broadphase layer
class MyObjectVsBroadPhaseLayerFilter : public JPH::ObjectVsBroadPhaseLayerFilter {
   public:
    virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override;
};
