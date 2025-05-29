#pragma once
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <unordered_map>
#include <unordered_set>
#include <string>

#include "renderer.h"
#include "input.h"
#include "physics.h"
#include "transform.h"
#include "animation.h"
#include "player.h"
#include "camera.h"
#include "shader.h"
#include "ecs.h"
#include "loader.h"
#include "editor.h"
#include "inspector.h"

struct Scene {
    bool menuOpen = false;
    bool menuCanOpen = true;

    double physicsAccum = 0.0f;
    double currentFrame = 0.0f;
    double lastFrame = 0.0f;
    double deltaTime;

    float gravity = -18.81f;

    uint32_t nextEntityID = 1;
    vec3 wrenchOffset = vec3(0.0f, -0.42f, 0.37f);

    std::string name = "default";
    std::string scenePath = "";

    InputActions* input;
    Player* player;
    DirectionalLight sun;

    JPH::PhysicsSystem* physicsSystem;
    JPH::BodyInterface* bodyInterface;
    JPH::TempAllocatorImpl* tempAllocator;
    JPH::JobSystemThreadPool* jobSystem;
    JPH::BroadPhaseLayerInterface* broad_phase_layer_interface;
    JPH::ObjectVsBroadPhaseLayerFilter* object_vs_broadphase_layer_filter;
    JPH::ObjectLayerPairFilter* object_vs_object_layer_filter;
    std::unordered_set<uint32_t> movingRigidbodies;

    std::vector<Entity> entities;
    std::vector<Transform> transforms;
    std::vector<MeshRenderer> meshRenderers;
    std::vector<Animator> animators;
    std::vector<RigidBody> rigidbodies;
    std::vector<PointLight> pointLights;
    std::vector<SpotLight> spotLights;
    std::vector<Camera*> cameras;

    std::unordered_map<uint32_t, size_t> entityIndexMap;
    std::unordered_map<uint32_t, size_t> transformIndexMap;
    std::unordered_map<uint32_t, size_t> meshRendererIndexMap;
    std::unordered_map<uint32_t, size_t> rigidbodyIndexMap;
    std::unordered_map<uint32_t, size_t> animatorIndexMap;
    std::unordered_map<uint32_t, size_t> pointLightIndexMap;
    std::unordered_map<uint32_t, size_t> spotLightIndexMap;
};

void clearScene(Scene* scene);