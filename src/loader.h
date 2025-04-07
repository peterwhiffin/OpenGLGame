#ifndef LOADER_H
#define LOADER_H

#include <vector>

struct Mesh {
    struct Vertex {
        glm::vec2 position;
        glm::vec2 normal;
        glm::vec1 texCoord;
    };
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

struct Model {
    std::vector<Mesh> meshes;
};

Model* loadModel(const char* path);
unsigned int loadTexture(const char* path);

#endif