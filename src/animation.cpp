#include "animation.h"
#include "transform.h"

void updateAnimators(Scene* scene) {
    for (int i = 0; i < scene->animators.size(); i++) {
        Animator* animator = &scene->animators[i];
        animator->playbackTime += scene->deltaTime;
        for (AnimationChannel* channel : animator->currentAnimation->channels) {
            if (animator->playbackTime >= channel->positions[animator->nextKeyPosition[channel]].time) {
                animator->nextKeyPosition[channel]++;
                if (animator->nextKeyPosition[channel] >= channel->positions.size()) {
                    animator->nextKeyPosition[channel] = 0;
                    animator->playbackTime = 0.0f;
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

            setLocalPosition(scene, animator->channelMap[channel], glm::mix(getLocalPosition(scene, animator->channelMap[channel]), scene->wrenchOffset + channel->positions[animator->nextKeyPosition[channel]].position, lerp));
        }
    }
}