#pragma once
#include <string>
#include <vector>
#include <unordered_map>

#include "forward.h"
#include "utils/mathutils.h"
#include "ecs.h"
constexpr char* resourcePath = "..\\resources\\";

struct Entity;
struct EntityGroup;

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

struct Resources {
    std::vector<Model*> models;
    std::unordered_map<std::string, Mesh*> meshMap;
    std::unordered_map<std::string, Animation*> animationMap;
    std::unordered_map<std::string, Material*> materialMap;
    std::unordered_map<std::string, Texture*> textureMap;
    std::unordered_map<std::string, Model*> modelMap;

    std::unordered_map<std::string, TextureSettings> textureImportMap;
    std::unordered_map<std::string, ModelSettings> modelImportMap;
    std::unordered_map<std::string, uint32_t> prefabMap;

    EntityGroup prefabGroup;
};

void loadResources(Resources* resources, RenderState* renderer);
GLuint loadTextureFromFile(const char* path, TextureSettings settings);
