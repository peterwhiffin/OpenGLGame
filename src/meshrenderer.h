#pragma once
#include <vector>
#include <unordered_map>
#include "utils/mathutils.h"

struct Mesh;
struct Material;
struct SubMesh;
struct BoneInfo;
struct Scene;
struct EntityGroup;

struct MeshRenderer {
    uint32_t entityID;
    uint32_t rootEntity;
    GLint vao;
    Mesh* mesh;
    bool boneMatricesSet = false;
    std::vector<Material*> materials;
    std::vector<SubMesh> subMeshes;
    std::vector<mat4> boneMatrices;
    std::unordered_map<uint32_t, BoneInfo> transformBoneMap;
};

void initializeMeshRenderer(EntityGroup* entities, MeshRenderer* meshRenderer);