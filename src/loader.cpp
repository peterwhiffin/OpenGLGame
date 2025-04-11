#include <glad/glad.h>
#include <vector>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <glm/glm.hpp>
#include <iostream>
#include <string>
#include <glm/gtc/type_ptr.hpp>
#include <unordered_map>

#include "utils/stb_image.h"
#include "loader.h"
#include "assimp/material.h"
#include "assimp/types.h"
#include "glm/ext/quaternion_float.hpp"
#include "shader.h"

void processNode(aiNode* node, const aiScene* scene, glm::mat4 parentTransform, Model* newModel, std::string* directory, std::vector<Texture>* allTextures, int layer);
void processMesh(aiMesh* mesh, const aiScene* scene, const glm::mat4& transform, Model* model, std::string* directory, std::vector<Texture>* allTextures);
Texture loadhhTexture(aiMaterial* mat, aiTextureType type, std::string* directory, std::vector<Texture>* allTextures, bool gamma);
void createMeshBuffers(Mesh* mesh);
int layerCount = 0;
Model* loadModel(std::string path, std::vector<Texture>* allTextures) {
    Assimp::Importer importer;
    std::string directory;

    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return nullptr;
    }

    directory = path.substr(0, path.find_last_of('/'));
    Model* newModel = new Model();
    newModel->parent = nullptr;
    newModel->name = scene->mName.C_Str();
    processNode(scene->mRootNode, scene, glm::mat4(1.0f), newModel, &directory, allTextures, 0);
    std::cout << "layers: " << layerCount << std::endl;
    return newModel;
}

void processNode(aiNode* node, const aiScene* scene, glm::mat4 parentTransform, Model* parentModel, std::string* directory, std::vector<Texture>* allTextures, int layer) {
    glm::mat4 nodeTransform = glm::transpose(glm::make_mat4(&node->mTransformation.a1));
    glm::mat4 globalTransform = parentTransform * nodeTransform;
    Model* targetParent = parentModel;
    layer++;
    if (node->mNumMeshes != 0) {
        Model* childModel;

        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            childModel = new Model();
            childModel->parent = parentModel;
            parentModel->children.push_back(childModel);

            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            processMesh(mesh, scene, globalTransform, childModel, directory, allTextures);
        }

        targetParent = childModel;
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, globalTransform, targetParent, directory, allTextures, layer);
    }

    if (layer > layerCount) {
        layerCount = layer;
    }
    /*
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            Model* siblingModel = new Model();
            siblingModel->parent = parentModel;
            parentModel->children.push_back(siblingModel);

            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            processMesh(mesh, scene, globalTransform, siblingModel, directory, allTextures);
        }

        for (unsigned int i = 0; i < parentModel->children.size(); i++) {
            // Model* childModel = new Model();
            // childModel->parent = newModel;
            // newModel->children.push_back(childModel);
            processNode(node->mChildren[i], scene, globalTransform, parentModel->children[i], directory, allTextures);
        } */
}
void processMesh(aiMesh* mesh, const aiScene* scene, const glm::mat4& transform, Model* model, std::string* directory, std::vector<Texture>* allTextures) {
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

    aiColor4D baseColor(1.0f, 1.0f, 1.0f, 1.0f);
    std::string name;
    Material newMaterial;
    newMaterial.shininess = 32.0f;

    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        Texture diffuseTexture;
        Texture specularTexture;
        newMaterial.shader = 1;
        material->Get(AI_MATKEY_COLOR_DIFFUSE, baseColor);
        newMaterial.baseColor = glm::vec4(baseColor.r, baseColor.g, baseColor.b, baseColor.a);
        diffuseTexture = loadhhTexture(material, aiTextureType_DIFFUSE, directory, allTextures, true);
        specularTexture = loadhhTexture(material, aiTextureType_SPECULAR, directory, allTextures, false);
        newMaterial.textures.push_back(diffuseTexture);
        newMaterial.textures.push_back(specularTexture);
        newMaterial.name = material->GetName().C_Str();
    } else {
        newMaterial.baseColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        newMaterial.name = "default";
        newMaterial.shader = 1;
    }

    Mesh newMesh;
    newMesh.vertices = vertices;
    newMesh.indices = indices;
    newMesh.name = mesh->mName.C_Str();
    newMesh.material = newMaterial;
    createMeshBuffers(&newMesh);
    model->mesh = newMesh;
    model->hasMesh = true;
}

void createMeshBuffers(Mesh* mesh) {
    glGenVertexArrays(1, &mesh->VAO);
    glGenBuffers(1, &mesh->VBO);
    glGenBuffers(1, &mesh->EBO);

    glBindVertexArray(mesh->VAO);

    glBindBuffer(GL_ARRAY_BUFFER, mesh->VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh->vertices.size() * sizeof(Vertex), &mesh->vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->indices.size() * sizeof(unsigned int), &mesh->indices[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(vertex_attribute_location::kVertexPosition);
    glVertexAttribPointer(vertex_attribute_location::kVertexPosition, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    glEnableVertexAttribArray(vertex_attribute_location::kVertexNormal);
    glVertexAttribPointer(vertex_attribute_location::kVertexNormal, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    glEnableVertexAttribArray(vertex_attribute_location::kVertexTexCoord);
    glVertexAttribPointer(vertex_attribute_location::kVertexTexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));

    glBindVertexArray(0);
}

Texture loadhhTexture(aiMaterial* mat, aiTextureType type, std::string* directory, std::vector<Texture>* allTextures, bool gamma) {
    Texture newTexture;
    newTexture.path = "default";
    newTexture.id = allTextures->at(0).id;

    if (mat->GetTextureCount(type) != 0) {
        aiString texPath;
        mat->GetTexture(type, 0, &texPath);

        for (unsigned int i = 0; i < allTextures->size(); i++) {
            if (std::strcmp(allTextures->at(i).path.data(), texPath.C_Str()) == 0) {
                newTexture.id = allTextures->at(i).id;
                newTexture.path = allTextures->at(i).path;
                return newTexture;
            }
        }

        std::string fullPath = *directory + '/' + texPath.C_Str();
        newTexture.path = texPath.C_Str();
        newTexture.id = loadTextureFromFile(fullPath.data(), gamma);
        allTextures->push_back(newTexture);
    }

    return newTexture;
}

unsigned int loadTextureFromFile(const char* path, bool gamma) {
    unsigned int textureID = 1;
    int width;
    int height;
    int componentCount;

    unsigned char* data = stbi_load(path, &width, &height, &componentCount, 0);

    if (data) {
        glGenTextures(1, &textureID);
        GLenum format = GL_RED;
        GLenum internalFormat = GL_RED;

        if (componentCount == 3) {
            format = GL_RGB;
            internalFormat = gamma ? GL_SRGB : GL_RGB;
        } else if (componentCount == 4) {
            format = GL_RGBA;
            internalFormat = gamma ? GL_SRGB_ALPHA : GL_RGBA;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    } else {
        std::cerr << "ERROR::TEXTURE_FAILED_TO_LOAD at: " << path << std::endl;
    }

    stbi_image_free(data);
    return textureID;
}