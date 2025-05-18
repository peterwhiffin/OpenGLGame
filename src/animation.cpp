#include "animation.h"
#include "transform.h"
#include <cmath>

void updateAnimators(Scene* scene) {
    Animator* animator;
    uint32_t nextPositionKey;
    uint32_t nextRotationKey;
    uint32_t channelID;
    int32_t prevIndex;

    float playbackTime;
    float prevTime;
    float totalDuration;
    float timeElapsed;
    float targetTime;
    float t;

    vec3 prevPosition;
    vec3 targetPosition;
    vec3 nextPosition;

    quat prevRotation;
    quat targetRotation;
    quat nextRotation;

    for (int i = 0; i < scene->animators.size(); i++) {
        animator = &scene->animators[i];
        animator->playbackTime += scene->deltaTime;
        playbackTime = animator->playbackTime;

        for (AnimationChannel* channel : animator->currentAnimation->channels) {
            nextPositionKey = channel->nextPositionKey;
            nextRotationKey = channel->nextRotationKey;
            channelID = animator->channelMap[channel];

            targetTime = channel->positions[nextPositionKey].time;

            if (playbackTime >= targetTime) {
                nextPositionKey++;

                if (nextPositionKey == channel->positions.size()) {
                    nextPositionKey = 0;
                }

                channel->nextPositionKey = nextPositionKey;
                targetTime = channel->positions[nextPositionKey].time;
            }

            prevIndex = nextPositionKey > 0 ? nextPositionKey - 1 : 0;
            prevTime = channel->positions[prevIndex].time;
            totalDuration = targetTime - prevTime;
            timeElapsed = playbackTime - prevTime;
            t = JPH::min(timeElapsed / totalDuration, 1.0f);

            prevPosition = channel->positions[prevIndex].position;
            targetPosition = channel->positions[nextPositionKey].position;
            nextPosition = lerp(prevPosition, targetPosition, t);
            setLocalPosition(scene, channelID, nextPosition);

            targetTime = channel->rotations[nextRotationKey].time;

            if (playbackTime >= targetTime) {
                nextRotationKey++;
                if (nextRotationKey >= channel->rotations.size()) {
                    nextRotationKey = 0;
                }

                channel->nextRotationKey = nextRotationKey;
                targetTime = channel->rotations[nextRotationKey].time;
            }

            prevIndex = nextRotationKey > 0 ? nextRotationKey - 1 : 0;
            prevTime = channel->rotations[prevIndex].time;
            totalDuration = targetTime - prevTime;
            timeElapsed = playbackTime - prevTime;
            t = JPH::min(timeElapsed / totalDuration, 1.0f);

            prevRotation = channel->rotations[prevIndex].rotation;
            targetRotation = channel->rotations[channel->nextRotationKey].rotation;
            nextRotation = prevRotation.SLERP(targetRotation, t);
            setLocalRotation(scene, channelID, nextRotation);
        }

        if (playbackTime >= animator->currentAnimation->duration) {
            animator->playbackTime = 0.0f;
        }
    }
}

void playAnimation(Animator* animator, std::string name) {
    assert(animator->animationMap.count(name));

    Animation* currentAnim = animator->currentAnimation;
    Animation* newAnim = animator->animationMap[name];

    if (currentAnim == newAnim) {
        return;
    }

    animator->playbackTime = 0.0f;
    currentAnim = newAnim;
    AnimationChannel* channel;

    for (int i = 0; i < currentAnim->channels.size(); i++) {
        channel = currentAnim->channels[i];
        channel->nextPositionKey = 0;
        channel->nextRotationKey = 0;
        channel->nextScaleKey = 0;
    }

    animator->currentAnimation = currentAnim;
}