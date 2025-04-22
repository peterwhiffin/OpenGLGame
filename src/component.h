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
#include <typeinfo>

struct Entity;
struct Mesh;
struct Transform;
struct Animation;
struct ModelNode;
struct AnimationChannel;
struct Model;
struct MeshRenderer;
struct BoxCollider;
struct Animator;
struct RigidBody;
struct Camera;
struct CameraController;
struct Player;
struct WindowData;
struct DirectionalLight;
struct Scene;

constexpr uint32_t INVALID_ID = 0xFFFFFFFF;

namespace component {
constexpr unsigned int kTransform = 0;
constexpr unsigned int kMeshRenderer = 1;
constexpr unsigned int kBoxCollider = 2;
constexpr unsigned int kRigidBody = 3;
constexpr unsigned int kAnimator = 4;
}  // namespace component

uint32_t getEntityID(Scene* scene);
Transform* addTransform(Scene* scene, uint32_t entityID);
Entity* getNewEntity(Scene* scene, std::string name);
MeshRenderer* addMeshRenderer(Scene* scene, uint32_t entityID);
BoxCollider* addBoxCollider(Scene* scene, uint32_t entityID);
RigidBody* addRigidbody(Scene* scene, uint32_t entityID);
Animator* addAnimator(Scene* scene, uint32_t entityID, Model* model);
uint32_t createEntityFromModel(Scene* scene, Model* model, ModelNode* node, uint32_t parentEntityID, bool addColliders);
Camera* addCamera(Scene* scene, uint32_t entityID, float fov, float aspectRatio, float nearPlane, float farPlane);

struct Transform {
    uint32_t entityID;
    uint32_t parentEntityID = INVALID_ID;
    std::vector<uint32_t> childEntityIds;
    glm::vec3 localPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::quat localRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 localScale = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::mat4 worldTransform = glm::mat4(1.0f);
};

struct Entity {
    uint32_t id;
    std::string name;
    bool isActive;
};

struct WindowData {
    unsigned int width;
    unsigned int height;
};

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

struct Texture {
    std::string path;
    unsigned int id;
};

struct Material {
    unsigned int shader;
    std::string name;
    std::vector<Texture> textures;
    glm::vec4 baseColor;
    float shininess;
};

struct SubMesh {
    Mesh* mesh;
    unsigned int indexOffset;
    unsigned int indexCount;
    Material material;
};

struct Mesh {
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<SubMesh*> subMeshes;
    glm::vec3 center;
    glm::vec3 extent;
    glm::vec3 min;
    glm::vec3 max;
    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;
};

struct ModelNode {
    std::string name;
    ModelNode* parent;
    glm::mat4 transform;
    std::vector<ModelNode*> children;
    Mesh* mesh;
};

struct Model {
    std::string name;
    std::vector<Mesh*> meshes;
    std::vector<Material*> materials;
    std::vector<Animation*> animations;
    std::unordered_map<ModelNode*, AnimationChannel*> channelMap;
    ModelNode* rootNode;
};

struct MeshRenderer {
    uint32_t entityID;
    Mesh* mesh;
};

struct KeyFramePosition {
    glm::vec3 position;
    float time;
};

struct KeyFrameRotation {
    glm::quat rotation;
    float time;
};

struct KeyFrameScale {
    glm::vec3 scale;
    float time;
};

struct AnimationChannel {
    std::string name;
    std::vector<KeyFramePosition> positions;
    std::vector<KeyFrameRotation> rotations;
    std::vector<KeyFrameScale> scales;
};

struct Animation {
    std::string name;
    float duration;
    std::vector<AnimationChannel*> channels;
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

struct Player {
    uint32_t entityID;
    CameraController* cameraController;
    float jumpHeight = 10.0f;
    bool isGrounded = false;
    float moveSpeed = 10.0f;
    float groundCheckDistance = 0.2f;
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

struct Scene {
    WindowData windowData;
    uint32_t nextEntityID = 0;

    DirectionalLight sun;

    std::vector<Texture> textures;
    std::vector<Transform> transforms;
    std::vector<MeshRenderer> renderers;
    std::vector<Entity> entities;
    std::vector<BoxCollider> boxColliders;
    std::vector<RigidBody> rigidbodies;
    std::vector<Animator> animators;
    std::vector<Camera*> cameras;

    std::unordered_map<uint32_t, size_t> entityIndices;
    std::unordered_map<uint32_t, size_t> transformIndices;
    std::unordered_map<uint32_t, size_t> rendererIndices;
    std::unordered_map<uint32_t, size_t> colliderIndices;
    std::unordered_map<uint32_t, size_t> rigidbodyIndices;
    std::unordered_map<uint32_t, size_t> animatorIndices;

    float gravity;
    float deltaTime;
};