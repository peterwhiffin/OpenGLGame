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

const JPH::uint cMaxBodies = 65536;
const JPH::uint cNumBodyMutexes = 0;
const JPH::uint cMaxBodyPairs = 65536;
const JPH::uint cMaxContactConstraints = 10240;
const JPH::uint cCollisionSteps = 1;
const float cDeltaTime = 1.0f / 60.0f;

void initPhysics(Scene* scene);
void updatePhysics(Scene* scene);
void destroyPhysicsSystem();

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