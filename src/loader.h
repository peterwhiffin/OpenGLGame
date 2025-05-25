#pragma once
#include <string>
#include <vector>
#include <unordered_map>

#include "forward.h"
#include "utils/mathutils.h"

constexpr char* resourcePath = "..\\resources\\";

struct ModelSettings {
    std::string path;
};

struct TextureSettings {
    std::string path;
    bool gamma = true;
    GLint filter = GL_NEAREST;
};

struct ModelNode {
    std::string name;
    ModelNode* parent;
    Mesh* mesh;
    mat4 transform;
    mat4 localTransform;
    std::vector<ModelNode*> children;
};

struct Model {
    std::string path;
    std::string name;
    ModelNode* rootNode;
    mat4 RootNodeTransform;
    std::vector<Mesh*> meshes;
    std::vector<Material*> materials;
    std::vector<Animation*> animations;
    std::unordered_map<ModelNode*, AnimationChannel*> channelMap;
};

void loadResources(Scene* scene);
GLuint loadTextureFromFile(const char* path, TextureSettings settings);