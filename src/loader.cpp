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

void processNode(aiNode* node, const aiScene* scene, glm::mat4 parentTransform, Model* model, ModelNode* parentNode, std::string* directory, std::vector<Texture>* allTextures, unsigned int shader);
Texture loadTexture(aiMaterial* mat, aiTextureType type, std::string* directory, std::vector<Texture>* allTextures, bool gamma);
void createMeshBuffers(Mesh* mesh);
void processSubMesh(aiMesh* mesh, const aiScene* scene, Mesh* parentMesh, const glm::mat4 transform, std::string* directory, std::vector<Texture>* allTextures, unsigned int shader);
int nodeCounter = 0;

Model* loadModel(std::string path, std::vector<Texture>* allTextures, unsigned int shader) {
    Assimp::Importer importer;
    std::string directory;

    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return nullptr;
    }

    directory = path.substr(0, path.find_last_of('/'));
    Model* newModel = new Model();
    std::string name = scene->mRootNode->mName.C_Str();
    name = name.substr(0, name.find_last_of('.'));
    newModel->name = name;
    ModelNode* rootNode = new ModelNode();
    rootNode->parent = nullptr;
    newModel->rootNode = rootNode;
    processNode(scene->mRootNode, scene, glm::mat4(1.0f), newModel, rootNode, &directory, allTextures, shader);
    std::cout << "nodes processed: " << nodeCounter << std::endl;
    return newModel;
}

void processNode(aiNode* node, const aiScene* scene, glm::mat4 parentTransform, Model* model, ModelNode* parentNode, std::string* directory, std::vector<Texture>* allTextures, unsigned int shader) {
    glm::mat4 nodeTransform = glm::transpose(glm::make_mat4(&node->mTransformation.a1));
    glm::mat4 globalTransform = parentTransform * nodeTransform;
    ModelNode* targetParent = parentNode;

    if (node->mNumMeshes != 0) {
        ModelNode* childNode = new ModelNode();
        childNode->parent = parentNode;
        parentNode->children.push_back(childNode);
        childNode->transform = globalTransform;

        childNode->name = node->mName.C_Str();
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            processSubMesh(mesh, scene, &childNode->mesh, globalTransform, directory, allTextures, shader);
        }

        model->meshes.push_back(&childNode->mesh);

        for (int i = 0; i < childNode->mesh.subMeshes.size(); i++) {
            model->materials.push_back(&childNode->mesh.subMeshes[i]->material);
        }

        createMeshBuffers(&childNode->mesh);
        targetParent = childNode;
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, globalTransform, model, targetParent, directory, allTextures, shader);
    }
}
void processSubMesh(aiMesh* mesh, const aiScene* scene, Mesh* parentMesh, const glm::mat4 transform, std::string* directory, std::vector<Texture>* allTextures, unsigned int shader) {
    SubMesh* subMesh = new SubMesh();
    subMesh->mesh = parentMesh;
    parentMesh->subMeshes.push_back(subMesh);

    size_t baseVertex = parentMesh->vertices.size();

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;

        glm::vec4 position(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z, 1.0f);
        // vertex.position = glm::vec3(transform * position);
        vertex.position = position;

        glm::vec4 normal(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z, 0.0f);
        // vertex.normal = glm::normalize(glm::vec3(transform * normal));
        vertex.normal = glm::normalize(normal);

        vertex.texCoord = glm::vec2(0.0f, 0.0f);

        if (mesh->mTextureCoords[0]) {
            vertex.texCoord.x = mesh->mTextureCoords[0][i].x;
            vertex.texCoord.y = mesh->mTextureCoords[0][i].y;
        }

        parentMesh->vertices.push_back(vertex);
    }

    subMesh->indexOffset = parentMesh->indices.size();

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];

        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            parentMesh->indices.push_back(face.mIndices[j] + baseVertex);
        }
    }

    subMesh->indexCount = parentMesh->indices.size() - subMesh->indexOffset;

    aiColor4D baseColor(1.0f, 1.0f, 1.0f, 1.0f);
    std::string name;
    Material newMaterial;
    newMaterial.shininess = 32.0f;

    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        Texture diffuseTexture;
        Texture specularTexture;
        newMaterial.shader = shader;
        material->Get(AI_MATKEY_SHININESS, newMaterial.shininess);
        material->Get(AI_MATKEY_COLOR_DIFFUSE, baseColor);
        newMaterial.baseColor = glm::vec4(baseColor.r, baseColor.g, baseColor.b, baseColor.a);
        diffuseTexture = loadTexture(material, aiTextureType_DIFFUSE, directory, allTextures, true);
        specularTexture = loadTexture(material, aiTextureType_SHININESS, directory, allTextures, false);
        newMaterial.textures.push_back(diffuseTexture);
        newMaterial.textures.push_back(specularTexture);
        newMaterial.name = material->GetName().C_Str();
    } else {
        newMaterial.baseColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        newMaterial.name = "default";
        newMaterial.shader = shader;
    }

    subMesh->material = newMaterial;
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

Texture loadTexture(aiMaterial* mat, aiTextureType type, std::string* directory, std::vector<Texture>* allTextures, bool gamma) {
    Texture newTexture;
    newTexture.path = "default";
    newTexture.id = type == aiTextureType_DIFFUSE ? allTextures->at(0).id : allTextures->at(0).id;

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
        // glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    } else {
        std::cerr << "ERROR::TEXTURE_FAILED_TO_LOAD at: " << path << std::endl;
    }

    stbi_image_free(data);
    return textureID;
}