#ifndef COMPONENT_H
#define COMPONENT_H

#include <glm/glm.hpp>
#include <vector>
#include <string>

#include "glm/ext/quaternion_float.hpp"

struct Entity;
struct Material;
struct Transform;

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

struct Texture {
    std::string path;
    unsigned int id;
};

struct Mesh {
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;
};

struct Model {
    std::vector<Mesh> meshes;
    std::vector<Material> materials;
};

struct Component {
    Entity* entity;
    Transform* transform;

    Component(Entity* newEntity);
};

struct Transform {
    glm::vec3 localPosition;
    glm::vec3 localEulerAngles;
    glm::vec3 localScale;

    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;
};

struct Entity {
    Transform transform;
    std::vector<Component*> components;
};

struct Material {
    unsigned int shader;
    std::string name;
    std::vector<Texture> textures;
    glm::vec3 baseColor;
};

struct MeshRenderer : Component {
    Mesh mesh;
    Material material;

    MeshRenderer(Entity* newEntity);
};

MeshRenderer* addMeshRenderer(Entity* entity);

#endif