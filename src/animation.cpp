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
    float lerp;
    float targetTime;

    glm::vec3 currentPosition;
    glm::vec3 targetPosition;
    glm::vec3 nextPosition;

    glm::quat currentRotation;
    glm::quat targetRotation;
    glm::quat nextRotation;

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

            prevTime = 0.0f;
            prevIndex = nextPositionKey - 1;

            if (prevIndex > 0) {
                prevTime = channel->positions[prevIndex].time;
            }

            totalDuration = targetTime - prevTime;
            timeElapsed = playbackTime - prevTime;
            lerp = glm::min(timeElapsed / totalDuration, 1.0f);

            currentPosition = getLocalPosition(scene, channelID);
            targetPosition = channel->positions[nextPositionKey].position;
            nextPosition = glm::mix(currentPosition, targetPosition, lerp);
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

            prevTime = 0.0f;
            prevIndex = nextRotationKey - 1;

            if (prevIndex > 0) {
                prevTime = channel->rotations[prevIndex].time;
            }

            totalDuration = targetTime - prevTime;
            timeElapsed = playbackTime - prevTime;
            lerp = glm::min(timeElapsed / totalDuration, 1.0f);

            currentRotation = getLocalRotation(scene, animator->channelMap[channel]);
            targetRotation = channel->rotations[channel->nextRotationKey].rotation;
            nextRotation = glm::slerp(currentRotation, targetRotation, 1.0f);
            setLocalRotation(scene, channelID, nextRotation);
        }

        if (playbackTime >= animator->currentAnimation->duration) {
            animator->playbackTime = 0.0f;
        }
    }
}

void playAnimation(Animator* animator, std::string name) {
    if (!animator->animationMap.count(name)) {
        std::cerr << "ERROR::MISSING_ANIMATION::No animation with name: " << name << std::endl;
        return;
    }

    Animation* currentAnim = animator->currentAnimation;
    Animation* newAnim = animator->animationMap[name];

    if (currentAnim != newAnim) {
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
}