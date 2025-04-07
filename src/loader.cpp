#include <glad/glad.h>
#include <vector>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <glm/glm.hpp>
#include <iostream>
#include <string>
#include <glm/gtc/type_ptr.hpp>

#include "utils/stb_image.h"
#include "loader.h"
#include "assimp/material.h"
#include "assimp/types.h"
#include "glm/ext/quaternion_float.hpp"
#include "utils/stb_image.h"
#include "shader.h"

void processNode(aiNode* node, const aiScene* scene, glm::mat4 parentTransform, Model newModel);
Mesh processMesh(aiMesh* mesh, const aiScene* scene, const glm::mat4& transform);
unsigned int loadTexture(aiMaterial* mat, aiTextureType type, std::string typeName, bool gamma = false);
Model* loadModel(std::string path) {
    Assimp::Importer importer;
    std::string directory;

    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return;
    }

    directory = path.substr(0, path.find_last_of('/'));
    Model newModel = Model();
    processNode(scene->mRootNode, scene, glm::mat4(1.0f), newModel);
    return &newModel;
}

void processNode(aiNode* node, const aiScene* scene, glm::mat4 parentTransform, Model* newModel) {
    glm::mat4 nodeTransform = glm::transpose(glm::make_mat4(&node->mTransformation.a1));
    glm::mat4 globalTransform = parentTransform * nodeTransform;

    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        newModel->meshes.push_back(processMesh(mesh, scene, globalTransform));
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, globalTransform, newModel);
    }
}
Mesh processMesh(aiMesh* mesh, const aiScene* scene, const glm::mat4& transform) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Material> materials;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;

        glm::vec4 position(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z, 1.0f);
        vertex.position = glm::vec3(transform * position);

        glm::vec4 normal(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z, 0.0f);
        vertex.normal = glm::normalize(glm::vec3(transform * normal));

        vertex.texCoord = glm::vec2(0.0f, 0.0f);

        if (mesh->mTextureCoords[0]) {
            vertex.texCoord.x = mesh->mTextureCoords[0][i].x;
            vertex.texCoord.y = mesh->mTextureCoords[0][i].y;
        }

        vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];

        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    aiColor3D baseColor(1.0f, 1.0f, 1.0f);

    std::string name;

    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        Material newMaterial = Material();
        unsigned int diffuseTexture;
        unsigned int specularTexture;

        newMaterial.shader = 1;
        // glUniform3fv(uniform_location::kBaseColor, 1, glm::vec3(1.0f, 1.0f, 1.0f))
        //  diffuseTexture = loadTexture(material, aiTextureType_DIFFUSE, true);
    }
}
unsigned int loadTexture(aiMaterial* mat, aiTextureType type, bool gamma = false) {
}