#pragma once
#include "utils/mathutils.h"

struct Scene;

struct KeyFramePosition {
    vec3 position;
    float time;
};

struct KeyFrameRotation {
    quat rotation;
    float time;
};

struct KeyFrameScale {
    vec3 scale;
    float time;
};

struct AnimationChannel {
    std::string name;
    std::vector<KeyFramePosition> positions;
    std::vector<KeyFrameRotation> rotations;
    std::vector<KeyFrameScale> scales;
    uint32_t nextPositionKey = 0;
    uint32_t nextRotationKey = 0;
    uint32_t nextScaleKey = 0;
};

struct Animation {
    std::string name;
    float duration;
    std::vector<AnimationChannel*> channels;
};

struct Animator {
    uint32_t entityID;
    uint32_t currentIndex = 0;
    float playbackTime = 0.0f;
    Animation* currentAnimation = nullptr;
    std::vector<Animation*> animations;
    std::unordered_map<std::string, Animation*> animationMap;
    std::unordered_map<AnimationChannel*, uint32_t> channelMap;
};

void updateAnimators(Scene* scene);
void playAnimation(Animator* animator, std::string name);