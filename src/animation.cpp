#include "animation.h"
#include "transform.h"

void processAnimators(Animator& animator, float deltaTime, glm::vec3 wrenchOffset) {
    animator.playbackTime += deltaTime;
    for (AnimationChannel* channel : animator.currentAnimation->channels) {
        if (animator.playbackTime >= channel->positions[animator.nextKeyPosition[channel]].time) {
            animator.nextKeyPosition[channel]++;
            if (animator.nextKeyPosition[channel] >= channel->positions.size()) {
                animator.nextKeyPosition[channel] = 0;
                animator.playbackTime = 0.0f;
            }
        }

        float currentTime = channel->positions[animator.nextKeyPosition[channel]].time;
        float prevTime = 0.0f;
        int prevIndex = animator.nextKeyPosition[channel] - 1;

        if (prevIndex >= 0) {
            prevTime = channel->positions[prevIndex].time;
        }

        float totalDuration = currentTime - prevTime;
        float timeElapsed = animator.playbackTime - prevTime;
        float lerp = glm::min(timeElapsed / totalDuration, 1.0f);

        setLocalPosition(animator.channelMap[channel], glm::mix(getLocalPosition(animator.channelMap[channel]), wrenchOffset + channel->positions[animator.nextKeyPosition[channel]].position, lerp));
    }
}