#include "physics.h"
#include "transform.h"

void updateRigidBodies(Scene* scene) {
    float t = scene->physicsAccum / scene->cDeltaTime;
    for (RigidBody& rigidbody : scene->rigidbodies) {
        if (scene->bodyInterface->GetObjectLayer(rigidbody.joltBody) == Layers::NON_MOVING) {
            continue;
        }

        if (scene->physicsTicked) {
            rigidbody.lastPosition = getPosition(scene, rigidbody.entityID);
            rigidbody.lastRotation = getRotation(scene, rigidbody.entityID);
        }

        vec3 newPos = lerp(rigidbody.lastPosition, scene->bodyInterface->GetPosition(rigidbody.joltBody), t);
        setPosition(scene, rigidbody.entityID, newPos);

        if (!rigidbody.rotationLocked) {
            quat newRot = rigidbody.lastRotation.SLERP(scene->bodyInterface->GetRotation(rigidbody.joltBody), t);
            setRotation(scene, rigidbody.entityID, newRot);
        }
    }

    return;
}