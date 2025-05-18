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

struct Scene {
    std::string name = "default";
    std::string scenePath = "";
    GLFWwindow* window;
    WindowData windowData;
    InputActions input;
    JPH::PhysicsSystem physicsSystem;
    JPH::BodyInterface* bodyInterface;
    JPH::TempAllocatorImpl* tempAllocator;
    JPH::JobSystemThreadPool* jobSystem;
    JPH::BroadPhaseLayerInterface* broad_phase_layer_interface;
    JPH::ObjectVsBroadPhaseLayerFilter* object_vs_broadphase_layer_filter;
    JPH::ObjectLayerPairFilter* object_vs_object_layer_filter;
    JPH::DebugRendererSimple* debugRenderer;

    uint32_t trashCanEntity;
    GLuint pickingFBO, pickingRBO, litFBO, litRBO, ssaoFBO;
    GLuint pickingTex, litColorTex, bloomSSAOTex, blurTex, ssaoNoiseTex, ssaoPosTex, ssaoNormalTex;
    GLuint blurFBO[2], blurSwapTex[2];
    GLuint fullscreenVAO, fullscreenVBO;
    GLuint editorFBO, editorRBO, editorTex;
    GLuint lightingShader, postProcessShader, blurShader, depthShader, ssaoShader, pickingShader, shadowBlurShader, debugShader;

    uint32_t nodeClicked = INVALID_ID;

    float FPS = 0.0f;
    float frameTime = 0.0f;
    float timeAccum = 0.0f;
    float physicsAccum = 0.0f;
    float frameCount = 0;
    float currentFrame = 0.0f;
    float lastFrame = 0.0f;
    float deltaTime;
    float gravity = -18.81f;
    float normalStrength = 1.06f;
    float exposure = 1.0f;
    float bloomThreshold = 0.39f;
    float bloomAmount = 0.1f;
    float ambient = 0.004f;
    float AORadius = 0.5f;
    float AOBias = 0.025f;
    float AOAmount = 1.0f;
    float AOPower = 2.0f;

    bool menuOpen = false;
    bool menuCanOpen = true;
    bool useDeferred = false;
    bool horizontalBlur = true;
    bool isPicking = false;
    bool canPick = true;
    bool canDelete = true;

    GLuint matricesUBO;
    GlobalUBO matricesUBOData;
    uint32_t nextEntityID = 1;
    vec3 wrenchOffset = vec3(0.0f, -0.42f, 0.37f);

    Model* trashcanModel;
    Model* testRoom;
    Model* wrench;
    Model* arms;
    Model* wrenchArms;
    Player* player;

    DirectionalLight sun;

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

    std::vector<Texture> textures;
    std::unordered_map<std::string, Mesh*> meshMap;
    std::unordered_map<std::string, Animation*> animationMap;
    std::unordered_map<std::string, Material*> materialMap;
    std::unordered_map<std::string, Texture> textureMap;

    std::vector<Model*> models;
    std::vector<vec3> ssaoKernel;
    std::vector<vec3> ssaoNoise;

    std::unordered_set<uint32_t> selectedEntities;
};
