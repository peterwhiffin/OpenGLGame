#ifndef COMPONENT_H
#define COMPONENT_H

#include <glm/glm.hpp>
#include <vector>

#include "glm/ext/quaternion_float.hpp"

struct Transform {
    glm::vec3 localPosition = glm::vec3(0, 0, 0);
    glm::vec3 localEulerAngles = glm::vec3(0, 0, 0);
    glm::vec3 localScale = glm::vec3(0, 0, 0);

    glm::vec3 position = glm::vec3(0, 0, 0);
    glm::quat rotation = glm::quat(1, 0, 0, 0);
    glm::vec3 scale = glm::vec3(0, 0, 0);
};

struct Material {
    unsigned int shader;
    std::vector<unsigned int> uniformLocations;
    std::vector<unsigned int> samplerLocations;
    std::vector<unsigned int> textures;
};

struct Mesh {
    struct Vertex {
        glm::vec2 position;
        glm::vec2 normal;
        glm::vec1 texCoord;
    };
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    Material material;
};

struct Model {
    Transform transform = Transform();
    std::vector<Mesh> meshes;
    unsigned int shader;
};

#endif