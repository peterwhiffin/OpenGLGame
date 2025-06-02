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

    std::vector<Entity> entities;
    std::vector<Transform> transforms;
    std::vector<MeshRenderer> meshRenderers;
    std::vector<Animator> animators;
    std::vector<RigidBody> rigidbodies;
    std::vector<PointLight> pointLights;
    std::vector<SpotLight> spotLights;
    std::vector<Camera> cameras;

    std::unordered_map<uint32_t, size_t> entityIndexMap;
    std::unordered_map<uint32_t, size_t> transformIndexMap;
    std::unordered_map<uint32_t, size_t> meshRendererIndexMap;
    std::unordered_map<uint32_t, size_t> rigidbodyIndexMap;
    std::unordered_map<uint32_t, size_t> animatorIndexMap;
    std::unordered_map<uint32_t, size_t> pointLightIndexMap;
    std::unordered_map<uint32_t, size_t> spotLightIndexMap;
    std::unordered_map<uint32_t, size_t> cameraIndexMap;
};

void loadResources(Resources* resources, RenderState* renderer);
GLuint loadTextureFromFile(const char* path, TextureSettings settings);