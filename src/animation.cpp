#include "animation.h"
#include "transform.h"
#include <cmath>

void updateAnimators(Scene* scene) {
    for (int i = 0; i < scene->animators.size(); i++) {
        Animator* animator = &scene->animators[i];

        /* if (animator->currentAnimation == nullptr) {
    continue;
} */
        animator->playbackTime += scene->deltaTime;

        for (AnimationChannel* channel : animator->currentAnimation->channels) {
            if (animator->playbackTime >= channel->positions[animator->nextKeyPosition[channel]].time) {
                animator->nextKeyPosition[channel]++;
                if (animator->nextKeyPosition[channel] >= channel->positions.size()) {
                    animator->nextKeyPosition[channel] = 0;
                    // animator->playbackTime = 0.0f;
                }
            }

            float currentTime = channel->positions[animator->nextKeyPosition[channel]].time;
            float prevTime = 0.0f;
            int prevIndex = animator->nextKeyPosition[channel] - 1;

            if (prevIndex >= 0) {
                prevTime = channel->positions[prevIndex].time;
            }

            float totalDuration = currentTime - prevTime;
            float timeElapsed = animator->playbackTime - prevTime;
            float lerp = glm::min(timeElapsed / totalDuration, 1.0f);

            glm::vec3 currentPosition = getLocalPosition(scene, animator->channelMap[channel]);
            glm::vec3 targetPosition = channel->positions[animator->nextKeyPosition[channel]].position;

            glm::vec3 nextPosition = glm::mix(currentPosition, targetPosition, lerp);

            if (std::isinf(nextPosition.x)) {
                // std::cout << "big inf" << std::endl;
            }

            setLocalPosition(scene, animator->channelMap[channel], nextPosition);

            if (animator->playbackTime >= channel->rotations[animator->nextKeyRotation[channel]].time) {
                animator->nextKeyRotation[channel]++;
                if (animator->nextKeyRotation[channel] >= channel->rotations.size()) {
                    animator->nextKeyRotation[channel] = 0;
                    // animator->playbackTime = 0.0f;
                }
            }

            currentTime = channel->rotations[animator->nextKeyRotation[channel]].time;
            prevTime = 0.0f;
            prevIndex = animator->nextKeyRotation[channel] - 1;

            if (prevIndex >= 0) {
                prevTime = channel->rotations[prevIndex].time;
            }

            totalDuration = currentTime - prevTime;
            timeElapsed = animator->playbackTime - prevTime;
            lerp = glm::min(timeElapsed / totalDuration, 1.0f);

            glm::quat currentRotation = getLocalRotation(scene, animator->channelMap[channel]);
            glm::quat targetRotation = channel->rotations[animator->nextKeyRotation[channel]].rotation;

            glm::quat nextRotation = glm::slerp(currentRotation, targetRotation, 1.0f);

            setLocalRotation(scene, animator->channelMap[channel], nextRotation);
        }

        if (animator->playbackTime >= animator->currentAnimation->duration) {
            animator->playbackTime = 0.0f;
        }
    }
}