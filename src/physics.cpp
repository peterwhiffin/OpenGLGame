#include <iostream>
#include <cstdarg>
#include <thread>

#include "physics.h"
#include "transform.h"
#include "scene.h"

using namespace JPH;

static void TraceImpl(const char* inFMT, ...) {
    // Format the message
    va_list list;
    va_start(list, inFMT);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), inFMT, list);
    va_end(list);
}

bool MyObjectLayerPairFilter::ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const {
    switch (inObject1) {
        case Layers::NON_MOVING:
            return inObject2 == Layers::MOVING;  // Non moving only collides with moving
        case Layers::MOVING:
            return true;  // Moving collides with everything
        default:
            // JPH_ASSERT(false);
            return false;
    }
}

MyBroadPhaseLayerInterface::MyBroadPhaseLayerInterface() {
    mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
    mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
}

uint MyBroadPhaseLayerInterface::GetNumBroadPhaseLayers() const {
    return BroadPhaseLayers::NUM_LAYERS;
}

BroadPhaseLayer MyBroadPhaseLayerInterface::GetBroadPhaseLayer(ObjectLayer inLayer) const {
    // JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
    return mObjectToBroadPhase[inLayer];
}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
const char* MyBroadPhaseLayerInterface::GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const {
    switch ((BroadPhaseLayer::Type)inLayer) {
        case (BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:
            return "NON_MOVING";
        case (BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:
            return "MOVING";
        default:
            // JPH_ASSERT(false);
            return "INVALID";
    }
}
#endif  // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

/// Class that determines if an object layer can collide with a broadphase layer
bool MyObjectVsBroadPhaseLayerFilter::ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const {
    switch (inLayer1) {
        case Layers::NON_MOVING:
            return inLayer2 == BroadPhaseLayers::MOVING;
        case Layers::MOVING:
            return true;
        default:
            // JPH_ASSERT(false);
            return false;
    }
}

#ifdef JPH_ENABLE_ASSERTS
// Callback for asserts, connect this to your own assert handler if you have one
static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, uint inLine) {
    return true;
};

#endif  // JPH_ENABLE_ASSERTS

void initPhysics(Scene* scene) {
    RegisterDefaultAllocator();
    Trace = TraceImpl;
    JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)
    Factory::sInstance = new Factory();
    RegisterTypes();
    PhysicsScene* physicsScene = &scene->physicsScene;
    physicsScene->physicsSystem = new JPH::PhysicsSystem();
    physicsScene->tempAllocator = new TempAllocatorImpl(10 * 1024 * 1024);
    physicsScene->jobSystem = new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);
    physicsScene->broad_phase_layer_interface = new MyBroadPhaseLayerInterface();
    physicsScene->object_vs_broadphase_layer_filter = new MyObjectVsBroadPhaseLayerFilter();
    physicsScene->object_vs_object_layer_filter = new MyObjectLayerPairFilter();
    physicsScene->physicsSystem->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, *physicsScene->broad_phase_layer_interface, *physicsScene->object_vs_broadphase_layer_filter, *physicsScene->object_vs_object_layer_filter);
    physicsScene->physicsSystem->SetGravity(vec3(0.0f, -18.0f, 0.0f));
    physicsScene->bodyInterface = &physicsScene->physicsSystem->GetBodyInterface();
}

void updatePhysicsBodyPositions(Scene* scene) {
    EntityGroup* entities = &scene->entities;
    JPH::BodyInterface* bodyInterface = scene->physicsScene.bodyInterface;
    const float t = scene->physicsAccum / cDeltaTime;
    for (uint32_t entityID : entities->movingRigidbodies) {
        RigidBody* rigidbody = getRigidbody(entities, entityID);

        const vec3 newPos = lerp(rigidbody->lastPosition, bodyInterface->GetPosition(rigidbody->joltBody), t);
        setPosition(entities, rigidbody->entityID, newPos);

        if (!rigidbody->rotationLocked) {
            const quat newRot = rigidbody->lastRotation.SLERP(bodyInterface->GetRotation(rigidbody->joltBody), t);
            setRotation(entities, rigidbody->entityID, newRot);
        }
    }
}

static void setPreviousTransforms(Scene* scene) {
    const JPH::BodyInterface* bodyInterface = scene->physicsScene.bodyInterface;
    EntityGroup* entities = &scene->entities;
    for (uint32_t entityID : entities->movingRigidbodies) {
        RigidBody* rigidbody = getRigidbody(entities, entityID);

        rigidbody->lastPosition = bodyInterface->GetPosition(rigidbody->joltBody);
        rigidbody->lastRotation = bodyInterface->GetRotation(rigidbody->joltBody);
    }
}

void updatePhysics(Scene* scene) {
    PhysicsScene* physicsScene = &scene->physicsScene;
    scene->physicsAccum += scene->deltaTime;
    if (scene->physicsAccum < cDeltaTime) {
        return;
    }

    setPreviousTransforms(scene);
    physicsScene->physicsSystem->Update(cDeltaTime, cCollisionSteps, physicsScene->tempAllocator, physicsScene->jobSystem);
    scene->physicsAccum -= cDeltaTime;
}

void destroyPhysicsSystem() {
    UnregisterTypes();
    delete Factory::sInstance;
    Factory::sInstance = nullptr;
}