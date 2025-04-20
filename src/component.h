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

struct Entity;
struct Mesh;
struct Transform;
struct Animation;
struct ModelNode;
struct AnimationChannel;
struct Model;
struct MeshRenderer;
struct BoxCollider;

unsigned int getEntityID(unsigned int& nextEntityID);
Entity* createEntityFromModel(Model* model, ModelNode* node, std::vector<MeshRenderer*>* renderers, Entity* parentEntity, bool first, unsigned int nextEntityID);

struct Transform {
    Entity* entity;
    Transform* parent;
    std::vector<Transform*> children;
    glm::vec3 localPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::quat localRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 localScale = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::mat4 worldTransform = glm::mat4(1.0f);
};

struct Entity {
    unsigned int id;
    std::string name;
    Transform transform;
};

struct Scene {
    std::vector<Entity> entities;
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
    Entity* entity;
    Transform* transform;
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
    Entity* entity;
    std::vector<Animation*> animations;
    Animation* currentAnimation;
    float playbackTime = 0.0f;
    std::unordered_map<AnimationChannel*, Transform*> channelMap;
    std::unordered_map<AnimationChannel*, int> nextKeyPosition;
    std::unordered_map<AnimationChannel*, int> nextKeyRotation;
    std::unordered_map<AnimationChannel*, int> nextKeyScale;
};

struct Camera {
    Entity* entity;
    Transform* transform;
    float fov;
    float aspectRatio;
    float nearPlane;
    float farPlane;
    glm::mat4 projectionMatrix;
    glm::mat4 viewMatrix;
};

struct CameraController {
    Entity* entity;
    Transform* transform;
    Camera* camera;
    Transform* cameraTarget;
    float pitch = 0;
    float yaw = 0;
    float sensitivity = .3;
    float moveSpeed = 10;
};

struct BoxCollider {
    Entity* entity;
    Transform* transform;
    bool isActive = true;
    glm::vec3 center;
    glm::vec3 extent;
};

struct RigidBody {
    Entity* entity;
    Transform* transform;
    glm::vec3 linearVelocity;
    float linearMagnitude;
    float linearDrag;
    float mass = 1.0f;
    float friction = 10.0f;
    BoxCollider* collider;
};

struct Player {
    Entity* entity;
    float jumpHeight = 10.0f;
    bool isGrounded = false;
    BoxCollider* collider;
    CameraController* cameraController;
    RigidBody* rigidbody;
    float moveSpeed = 10.0f;
    float groundCheckDistance = 0.2f;
};

struct SpotLight {
    Entity* entity;
    Transform* transform;
    float range;
    float radius;
};

struct DirectionalLight {
    Entity* entity;
    Transform* transform;
    glm::vec3 position;
    glm::vec3 lookDirection;
    glm::vec3 color;
    glm::vec3 brightness;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};