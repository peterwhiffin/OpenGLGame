#pragma once
#include <glm/glm.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <string>
#include <vector>
#include <unordered_map>

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

Model* loadModel(std::string path, std::vector<Texture>* allTextures, unsigned int shader);
unsigned int loadTextureFromFile(const char* path, bool gamma);