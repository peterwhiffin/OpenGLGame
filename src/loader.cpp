#define GLM_ENABLE_EXPERIMENTAL
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
#include <glm/gtx/string_cast.hpp>

#include "utils/stb_image.h"
#include "loader.h"
#include "assimp/material.h"
#include "assimp/types.h"
#include "glm/ext/quaternion_float.hpp"
#include "shader.h"

ModelNode* processNode(aiNode* node, const aiScene* scene, glm::mat4 parentTransform, Model* model, ModelNode* parentNode, std::string* directory, std::vector<Texture>* allTextures, unsigned int shader);
Texture loadTexture(aiMaterial* mat, aiTextureType type, std::string* directory, std::vector<Texture>* allTextures, bool gamma);
void createMeshBuffers(Mesh* mesh);
void processSubMesh(aiMesh* mesh, const aiScene* scene, Mesh* parentMesh, const glm::mat4 transform, std::string* directory, std::vector<Texture>* allTextures, unsigned int shader);
void processAnimations(const aiScene* scene, Model* model);

Model* loadModel(std::string path, std::vector<Texture>* allTextures, unsigned int shader) {
    Assimp::Importer importer;
    std::string directory;

    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return nullptr;
    }

    directory = path.substr(0, path.find_last_of('/'));
    std::string name = scene->mRootNode->mName.C_Str();
    name = name.substr(0, name.find_last_of('.'));

    Model* newModel = new Model();
    newModel->name = name;
    processAnimations(scene, newModel);
    newModel->rootNode = processNode(scene->mRootNode, scene, glm::mat4(1.0f), newModel, nullptr, &directory, allTextures, shader);
    return newModel;
}

void processAnimations(const aiScene* scene, Model* model) {
    for (int i = 0; i < scene->mNumAnimations; i++) {
        aiAnimation* aiAnim = scene->mAnimations[i];
        Animation* newAnimation = new Animation();
        newAnimation->name = aiAnim->mName.C_Str();
        newAnimation->duration = aiAnim->mDuration / aiAnim->mTicksPerSecond;

        for (int j = 0; j < aiAnim->mNumChannels; j++) {
            AnimationChannel channel;
            channel.name = aiAnim->mChannels[j]->mNodeName.C_Str();

            std::cout << channel.name << std::endl;
            for (int k = 0; k < aiAnim->mChannels[j]->mNumPositionKeys; k++) {
                aiVector3D aiPosition(aiAnim->mChannels[j]->mPositionKeys[k].mValue);
                glm::vec3 position(aiPosition.x, aiPosition.y, aiPosition.z);
                KeyFramePosition keyFrame;
                keyFrame.position = position;
                keyFrame.time = aiAnim->mChannels[j]->mPositionKeys[k].mTime / aiAnim->mTicksPerSecond;
                channel.positions.push_back(keyFrame);
            }

            for (int k = 0; k < aiAnim->mChannels[j]->mNumRotationKeys; k++) {
                aiQuaternion aiRotation(aiAnim->mChannels[j]->mRotationKeys[k].mValue);
                glm::quat rotation(aiRotation.w, aiRotation.x, aiRotation.y, aiRotation.z);
                KeyFrameRotation keyFrame;
                keyFrame.rotation = rotation;
                keyFrame.time = aiAnim->mChannels[j]->mRotationKeys[k].mTime / aiAnim->mTicksPerSecond;
                channel.rotations.push_back(keyFrame);
            }

            for (int k = 0; k < aiAnim->mChannels[j]->mNumScalingKeys; k++) {
                aiVector3D aiScale(aiAnim->mChannels[j]->mScalingKeys[k].mValue);
                glm::vec3 scale(aiScale.x, aiScale.y, aiScale.z);
                KeyFrameScale keyFrame;
                keyFrame.scale = scale;
                keyFrame.time = aiAnim->mChannels[j]->mScalingKeys[k].mTime / aiAnim->mTicksPerSecond;
                channel.scales.push_back(keyFrame);
            }

            newAnimation->channels.push_back(channel);
        }

        model->animations.push_back(newAnimation);
    }
}

ModelNode* processNode(aiNode* node, const aiScene* scene, glm::mat4 parentTransform, Model* model, ModelNode* parentNode, std::string* directory, std::vector<Texture>* allTextures, unsigned int shader) {
    glm::mat4 nodeTransform = glm::transpose(glm::make_mat4(&node->mTransformation.a1));
    glm::mat4 globalTransform = parentTransform * nodeTransform;
    ModelNode* childNode = new ModelNode();
    childNode->transform = globalTransform;
    childNode->parent = parentNode;
    childNode->name = node->mName.C_Str();
    childNode->mesh = nullptr;

    for (Animation* animation : model->animations) {
        for (AnimationChannel channel : animation->channels) {
            if (node->mName.C_Str() == channel.name) {
                model->channelMap[childNode] = &channel;
            }
        }
    }

    if (node->mNumMeshes != 0) {
        childNode->mesh = new Mesh();
        childNode->mesh->min = glm::vec3(scene->mMeshes[node->mMeshes[0]]->mVertices[0].x, scene->mMeshes[node->mMeshes[0]]->mVertices[0].y, scene->mMeshes[node->mMeshes[0]]->mVertices[0].z);
        childNode->mesh->max = childNode->mesh->min;

        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            processSubMesh(mesh, scene, childNode->mesh, globalTransform, directory, allTextures, shader);
        }

        childNode->mesh->center = (childNode->mesh->min + childNode->mesh->max) * 0.5f;
        childNode->mesh->extent = (childNode->mesh->max - childNode->mesh->min) * 0.5f;

        model->meshes.push_back(childNode->mesh);

        for (int i = 0; i < childNode->mesh->subMeshes.size(); i++) {
            model->materials.push_back(&childNode->mesh->subMeshes[i]->material);
        }

        createMeshBuffers(childNode->mesh);
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        childNode->children.push_back(processNode(node->mChildren[i], scene, globalTransform, model, childNode, directory, allTextures, shader));
    }

    return childNode;
}
void processSubMesh(aiMesh* mesh, const aiScene* scene, Mesh* parentMesh, const glm::mat4 transform, std::string* directory, std::vector<Texture>* allTextures, unsigned int shader) {
    SubMesh* subMesh = new SubMesh();
    subMesh->mesh = parentMesh;
    parentMesh->subMeshes.push_back(subMesh);

    size_t baseVertex = parentMesh->vertices.size();

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;

        vertex.position = glm::vec4(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z, 1.0f);
        vertex.normal = glm::normalize(glm::vec4(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z, 0.0f));
        vertex.texCoord = glm::vec2(0.0f, 0.0f);

        parentMesh->min = glm::min(parentMesh->min, vertex.position);
        parentMesh->max = glm::max(parentMesh->max, vertex.position);

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
    Material newMaterial;
    newMaterial.shininess = 32.0f;

    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        Texture diffuseTexture = loadTexture(material, aiTextureType_DIFFUSE, directory, allTextures, true);
        Texture specularTexture = loadTexture(material, aiTextureType_SHININESS, directory, allTextures, false);

        material->Get(AI_MATKEY_COLOR_DIFFUSE, baseColor);
        material->Get(AI_MATKEY_SHININESS, newMaterial.shininess);

        newMaterial.shader = shader;
        newMaterial.baseColor = glm::vec4(baseColor.r, baseColor.g, baseColor.b, baseColor.a);
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
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    } else {
        std::cerr << "ERROR::TEXTURE_FAILED_TO_LOAD at: " << path << std::endl;
    }

    stbi_image_free(data);
    return textureID;
}