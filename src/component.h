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

glm::vec3 right(Transform* transform);
glm::vec3 up(Transform* transform);
glm::vec3 forward(Transform* transform);
glm::vec3 QuaternionByVector3(glm::quat rotation, glm::vec3 point);

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

    Transform(Entity* entity);
};

struct Mesh {
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;
    Material material;
};

struct Model {
    std::vector<Mesh> meshes;
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
    Camera& camera;
    float pitch = 0;
    float yaw = 0;
    float sensitivity = .3;
    float moveSpeed = 15;

    CameraController(Entity* entity, Camera& camera);
};

struct Player : Component {
};

struct MeshRenderer : Component {
    Mesh* mesh;
    Material* material;

    MeshRenderer(Entity* entity, Mesh* mesh, Material* material);
};

struct Entity {
    Transform transform;
    std::vector<Component*> components;

    Entity();
};

#endif