#pragma once
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <unordered_map>
#include <vector>
#include <string>
#include <iostream>

#include "loader.h"

constexpr uint32_t INVALID_ID = 0xFFFFFFFF;

struct Entity {
    uint32_t id;
    std::string name;
    bool isActive;
};

struct Transform {
    uint32_t entityID;
    uint32_t parentEntityID = INVALID_ID;
    std::vector<uint32_t> childEntityIds;
    glm::vec3 localPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::quat localRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 localScale = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::mat4 worldTransform = glm::mat4(1.0f);
};

struct MeshRenderer {
    uint32_t entityID;
    Mesh* mesh;
};

struct BoxCollider {
    uint32_t entityID;
    bool isActive = true;
    glm::vec3 center;
    glm::vec3 extent;
};

struct RigidBody {
    uint32_t entityID;
    glm::vec3 linearVelocity;
    float linearMagnitude;
    float linearDrag;
    float mass = 1.0f;
    float friction = 10.0f;
};

struct Animator {
    uint32_t entityID;
    std::vector<Animation*> animations;
    Animation* currentAnimation;
    float playbackTime = 0.0f;
    std::unordered_map<AnimationChannel*, uint32_t> channelMap;
    std::unordered_map<AnimationChannel*, int> nextKeyPosition;
    std::unordered_map<AnimationChannel*, int> nextKeyRotation;
    std::unordered_map<AnimationChannel*, int> nextKeyScale;
};

struct Camera {
    uint32_t entityID;
    float fov;
    float aspectRatio;
    float nearPlane;
    float farPlane;
    glm::mat4 projectionMatrix;
    glm::mat4 viewMatrix;
};

struct CameraController {
    uint32_t entityID;
    uint32_t cameraTargetEntityID;
    Camera* camera;
    float pitch = 0;
    float yaw = 0;
    float sensitivity = .3;
    float moveSpeed = 10;
};

struct SpotLight {
    uint32_t entityID;
    float range;
    float radius;
};

struct DirectionalLight {
    uint32_t entityID;
    glm::vec3 position;
    glm::vec3 lookDirection;
    glm::vec3 color;
    glm::vec3 brightness;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float ambientBrightness;
    float diffuseBrightness;

    bool isEnabled;
};

struct PointLight {
    uint32_t entityID;
    unsigned int isActive;
    float constant;
    float linear;
    float quadratic;
    float brightness;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct WindowData {
    unsigned int width;
    unsigned int height;
};

struct Scene {
    WindowData windowData;

    unsigned int gBuffer, gPosition, gNormal, gColorSpec, gDepth;
    unsigned int fullscreenVAO, fullscreenVBO;
    unsigned int litForward, geometryPass, deferredLightingPass;

    float FPS = 0.0f;
    float frameTime = 0.0f;
    float timeAccum = 0.0f;
    float frameCount = 0;
    float currentFrame = 0.0f;
    float lastFrame = 0.0f;
    float deltaTime;
    float gravity;
    float normalStrength = 1.0f;

    bool menuOpen = false;
    bool menuCanOpen = true;
    bool useDeferred = false;

    uint32_t nextEntityID = 1;
    glm::vec3 wrenchOffset = glm::vec3(0.3f, -0.3f, -0.5f);

    Model* trashcanModel;

    DirectionalLight sun;
    std::vector<PointLight> pointLights;
    std::vector<Texture> textures;
    std::vector<Entity> entities;
    std::vector<Transform> transforms;
    std::vector<MeshRenderer> meshRenderers;
    std::vector<BoxCollider> boxColliders;
    std::vector<RigidBody> rigidbodies;
    std::vector<Animator> animators;
    std::vector<Camera*> cameras;

    std::unordered_map<uint32_t, size_t> entityIndexMap;
    std::unordered_map<uint32_t, size_t> transformIndexMap;
    std::unordered_map<uint32_t, size_t> meshRendererIndexMap;
    std::unordered_map<uint32_t, size_t> boxColliderIndexMap;
    std::unordered_map<uint32_t, size_t> rigidbodyIndexMap;
    std::unordered_map<uint32_t, size_t> animatorIndexMap;
    std::unordered_map<uint32_t, size_t> pointLightIndexMap;
};

uint32_t getEntityID(Scene* scene);
Transform* addTransform(Scene* scene, uint32_t entityID);
Entity* getNewEntity(Scene* scene, std::string name);
MeshRenderer* addMeshRenderer(Scene* scene, uint32_t entityID);
BoxCollider* addBoxCollider(Scene* scene, uint32_t entityID);
RigidBody* addRigidbody(Scene* scene, uint32_t entityID);
Animator* addAnimator(Scene* scene, uint32_t entityID, Model* model);
uint32_t createEntityFromModel(Scene* scene, ModelNode* node, uint32_t parentEntityID, bool addColliders);
Camera* addCamera(Scene* scene, uint32_t entityID, float fov, float aspectRatio, float nearPlane, float farPlane);
PointLight* addPointLight(Scene* scene, uint32_t entityID);

Entity* getEntity(Scene* scene, uint32_t entityID);
Transform* getTransform(Scene* scene, uint32_t entityID);
MeshRenderer* getMeshRenderer(Scene* scene, uint32_t entityID);
BoxCollider* getBoxCollider(Scene* scene, uint32_t entityID);
RigidBody* getRigidbody(Scene* scene, uint32_t entityID);
Animator* getAnimator(Scene* scene, uint32_t entityID);
PointLight* getPointLight(Scene* scene, uint32_t entityID);