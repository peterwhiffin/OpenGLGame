#ifndef COMPONENT_H
#define COMPONENT_H

#include <glm/glm.hpp>
#include <vector>

#include "loader.h"
#include "glm/ext/quaternion_float.hpp"

struct Transform : Component {
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

struct Component {
    Entity* entity;
    Transform* transform;

    Component(Entity* newEntity);
};

struct Material {
    unsigned int shader;
    std::vector<unsigned int> uniformLocations;
    std::vector<unsigned int> samplerLocations;
    std::vector<unsigned int> textures;
};

struct MeshRenderer : Component {
    Mesh mesh;
    Material material;

    MeshRenderer(Entity* newEntity);
};

MeshRenderer* addMeshRenderer(Entity* entity);

#endif