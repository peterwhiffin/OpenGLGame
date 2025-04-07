#ifndef LOADER_H
#define LOADER_H

#include <vector>
#include <string>

#include "component.h"

struct Vertex {
    glm::vec2 position;
    glm::vec2 normal;
    glm::vec1 texCoord;
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

struct Model {
    std::vector<Mesh> meshes;
    std::vector<Material> materials;
};

Model* loadModel(std::string path);
unsigned int loadTexture(const char* path);

#endif