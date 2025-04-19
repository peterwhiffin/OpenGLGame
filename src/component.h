#ifndef COMPONENT_H
#define COMPONENT_H
#include <glm/ext/quaternion_float.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>

#include "glm/ext/quaternion_float.hpp"

struct Entity;
struct Component;
struct Transform;
struct SubMesh;
struct Mesh;
struct BoxCollider;
struct RigidBody;

glm::vec3 right(Transform* transform);
glm::vec3 up(Transform* transform);
glm::vec3 forward(Transform* transform);
glm::vec3 QuaternionByVector3(glm::quat rotation, glm::vec3 point);

void updateTransformMatrices(Transform& transform);

glm::vec3 getPosition(Transform& transform);
glm::quat getRotation(Transform& transform);
glm::vec3 getScale(Transform& transform);
glm::quat quatFromMatrix(glm::mat4& matrix);

void setLocalPosition(Transform& transform, glm::vec3 localPosition);
void setLocalRotation(Transform& transform, glm::quat localRotation);
void setLocalScale(Transform& transform, glm::vec3 localScale);
void setPosition(Transform& transform, glm::vec3 position);
void setRotation(Transform& transform, glm::quat rotation);
void setScale(Transform& transform, glm::vec3 scale);
void setParent(Transform& child, Transform& parent);
void removeParent(Transform& transform);

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

struct Component {
    Entity* entity;
    Transform* transform;
    Component(Entity* entity);
};

struct Transform : Component {
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::mat4 localToWorldMatrix = glm::mat4(1.0f);
    Transform(Entity* entity);
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
    ModelNode* rootNode;
};

struct Camera : Component {
    float fov;
    float aspectRatio;
    float nearPlane;
    float farPlane;
    glm::mat4 projectionMatrix;
    glm::mat4 viewMatrix;

    Camera(Entity* entity, float fov, float aspectRatio, float nearPlane, float farPlane);
};

struct CameraController : Component {
    Camera* camera;
    Transform* cameraTarget;
    float pitch = 0;
    float yaw = 0;
    float sensitivity = .3;
    float moveSpeed = 10;

    CameraController(Entity* entity, Camera* camera);
};

struct Player : Component {
    float jumpHeight = 10.0f;
    bool isGrounded = false;
    BoxCollider* collider;
    CameraController* cameraController;
    RigidBody* rigidbody;
    float moveSpeed = 10.0f;
    float groundCheckDistance = 0.2f;
    Player(Entity* entity);
};

struct MeshRenderer : Component {
    Mesh* mesh;
    MeshRenderer(Entity* entity, Mesh* mesh);
};

struct BoxCollider : Component {
    bool isActive = true;
    glm::vec3 center;
    glm::vec3 extent;
    BoxCollider(Entity* entity);
};

struct RigidBody : Component {
    glm::vec3 linearVelocity;
    float linearMagnitude;
    float linearDrag;
    float mass = 1.0f;
    float friction = 10.0f;
    BoxCollider* collider;
    RigidBody(Entity* entity);
};

struct SpotLight : Component {
    float range;
    float radius;
    SpotLight(Entity* entity);
};

struct Entity {
    Transform transform;
    unsigned int id;
    std::string name;
    Entity* parent;
    std::vector<Entity*> children;
    std::vector<Component*> components;

    Entity();
};

#endif