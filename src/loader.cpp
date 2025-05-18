#define GLM_ENABLE_EXPERIMENTAL
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <iostream>

#include "utils/stb_image.h"
#include "loader.h"
#include "assimp/material.h"
#include "assimp/types.h"
#include "glm/ext/quaternion_float.hpp"
#include "shader.h"
#include "transform.h"

void processAnimations(Scene* gameScene, const aiScene* scene, Model* model) {
    for (uint32_t i = 0; i < scene->mNumAnimations; i++) {
        aiAnimation* aiAnim = scene->mAnimations[i];
        Animation* newAnimation = new Animation();
        newAnimation->name = aiAnim->mName.C_Str();
        newAnimation->duration = aiAnim->mDuration / aiAnim->mTicksPerSecond;

        for (uint32_t j = 0; j < aiAnim->mNumChannels; j++) {
            AnimationChannel* channel = new AnimationChannel();
            channel->name = aiAnim->mChannels[j]->mNodeName.C_Str();

            for (uint32_t k = 0; k < aiAnim->mChannels[j]->mNumPositionKeys; k++) {
                aiVector3D aiPosition(aiAnim->mChannels[j]->mPositionKeys[k].mValue);
                vec3 position(aiPosition.x, aiPosition.y, aiPosition.z);
                KeyFramePosition keyFrame;
                keyFrame.position = position;
                keyFrame.time = aiAnim->mChannels[j]->mPositionKeys[k].mTime / aiAnim->mTicksPerSecond;
                channel->positions.push_back(keyFrame);
            }

            for (uint32_t k = 0; k < aiAnim->mChannels[j]->mNumRotationKeys; k++) {
                aiQuaternion aiRotation(aiAnim->mChannels[j]->mRotationKeys[k].mValue);
                quat rotation(aiRotation.x, aiRotation.y, aiRotation.z, aiRotation.w);
                KeyFrameRotation keyFrame;
                keyFrame.rotation = rotation;
                keyFrame.time = aiAnim->mChannels[j]->mRotationKeys[k].mTime / aiAnim->mTicksPerSecond;
                channel->rotations.push_back(keyFrame);
            }

            for (uint32_t k = 0; k < aiAnim->mChannels[j]->mNumScalingKeys; k++) {
                aiVector3D aiScale(aiAnim->mChannels[j]->mScalingKeys[k].mValue);
                vec3 scale(aiScale.x, aiScale.y, aiScale.z);
                KeyFrameScale keyFrame;
                keyFrame.scale = scale;
                keyFrame.time = aiAnim->mChannels[j]->mScalingKeys[k].mTime / aiAnim->mTicksPerSecond;
                channel->scales.push_back(keyFrame);
            }

            newAnimation->channels.push_back(channel);
        }

        model->animations.push_back(newAnimation);
        gameScene->animationMap[newAnimation->name] = newAnimation;
    }
}

void createMeshBuffers(Mesh* mesh) {
    glGenVertexArrays(1, &mesh->VAO);
    glGenBuffers(1, &mesh->VBO);
    glGenBuffers(1, &mesh->EBO);

    glBindVertexArray(mesh->VAO);

    glBindBuffer(GL_ARRAY_BUFFER, mesh->VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh->vertices.size() * sizeof(Vertex), &mesh->vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->indices.size() * sizeof(GLuint), &mesh->indices[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(vertex_attribute_location::kVertexPosition);
    glVertexAttribPointer(vertex_attribute_location::kVertexPosition, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    glEnableVertexAttribArray(vertex_attribute_location::kVertexTexCoord);
    glVertexAttribPointer(vertex_attribute_location::kVertexTexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));

    glEnableVertexAttribArray(vertex_attribute_location::kVertexNormal);
    glVertexAttribPointer(vertex_attribute_location::kVertexNormal, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    glEnableVertexAttribArray(vertex_attribute_location::kVertexTangent);
    glVertexAttribPointer(vertex_attribute_location::kVertexTangent, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));

    glEnableVertexAttribArray(vertex_attribute_location::kVertexBoneIDs);
    glVertexAttribIPointer(vertex_attribute_location::kVertexBoneIDs, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, boneIDs));

    glEnableVertexAttribArray(vertex_attribute_location::kVertexWeights);
    glVertexAttribPointer(vertex_attribute_location::kVertexWeights, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, weights));

    glBindVertexArray(0);
}

GLuint loadTextureFromFile(const char* path, bool gamma, bool isNormal, GLint filter) {
    GLuint textureID = 1;
    GLsizei width;
    GLsizei height;
    int componentCount;

    unsigned char* data = stbi_load(path, &width, &height, &componentCount, 0);

    if (data) {
        glGenTextures(1, &textureID);
        GLenum format = GL_RED;
        GLenum internalFormat = GL_RED;
        GLenum dataType = GL_UNSIGNED_BYTE;

        if (componentCount == 3) {
            format = GL_RGB;
            internalFormat = gamma ? GL_SRGB : GL_RGB;
            // internalFormat = GL_RGB;
        } else if (componentCount == 4) {
            format = GL_RGBA;
            internalFormat = gamma ? GL_SRGB_ALPHA : GL_RGBA;
            // internalFormat = GL_RGBA;
        }

        /*         if (isNormal) {
                    format = GL_RGBA;
                    internalFormat = GL_RGBA;
                    dataType = GL_UNSIGNED_BYTE;
                }
         */
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, dataType, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

    } else {
        std::cerr << "ERROR::TEXTURE_FAILED_TO_LOAD at: " << path << std::endl;
    }

    stbi_image_free(data);
    return textureID;
}

Texture loadTexture(Scene* gameScene, aiMaterial* mat, aiTextureType type, std::string* directory, std::vector<Texture>* allTextures, bool whiteIsDefault, bool gamma) {
    Texture newTexture;
    newTexture.path = "default";
    newTexture.name = "white";
    uint32_t defaultTex = whiteIsDefault ? 0 : 1;
    GLint filter = GL_NEAREST;
    bool isNormal = false;

    switch (type) {
        case aiTextureType_DIFFUSE:  // albedo map
            newTexture.id = allTextures->at(1).id;
            break;
        case aiTextureType_METALNESS:  // roughness map
            newTexture.id = allTextures->at(1).id;
            break;
        case aiTextureType_DIFFUSE_ROUGHNESS:  // metallic map
            newTexture.id = allTextures->at(1).id;
            break;
        case aiTextureType_AMBIENT_OCCLUSION:  // ao map
            newTexture.id = allTextures->at(1).id;
            break;
        case aiTextureType_NORMALS:  // normal map
            newTexture.id = allTextures->at(2).id;
            filter = GL_NEAREST;
            isNormal = true;
            break;
    }

    if (mat->GetTextureCount(type) != 0) {
        aiString texPath;
        mat->GetTexture(type, 0, &texPath);

        for (uint32_t i = 0; i < allTextures->size(); i++) {
            if (std::strcmp(allTextures->at(i).path.data(), texPath.C_Str()) == 0) {
                newTexture.id = allTextures->at(i).id;
                newTexture.path = allTextures->at(i).path;
                return newTexture;
            }
        }

        std::string fullPath = *directory + '/' + texPath.C_Str();
        size_t offset = fullPath.find_last_of('/');
        newTexture.path = texPath.C_Str();
        newTexture.name = fullPath.substr(offset + 1, fullPath.length() - offset);
        newTexture.id = loadTextureFromFile(fullPath.data(), gamma, isNormal, filter);
        allTextures->push_back(newTexture);
        gameScene->textureMap[newTexture.name] = newTexture;
    }

    return newTexture;
}

inline vec3 min(const vec3& a, const vec4& b) {
    return vec3(
        std::min(a.GetX(), b.GetX()),
        std::min(a.GetY(), b.GetY()),
        std::min(a.GetZ(), b.GetZ()));
}

inline vec3 max(const vec3& a, const vec4& b) {
    return vec3(
        std::max(a.GetX(), b.GetX()),
        std::max(a.GetY(), b.GetY()),
        std::max(a.GetZ(), b.GetZ()));
}

void processSubMesh(Scene* gameScene, aiMesh* mesh, const aiScene* scene, Mesh* parentMesh, const mat4 transform, std::string* directory, std::vector<Texture>* allTextures, GLuint shader, bool whiteIsDefault) {
    SubMesh subMesh;

    size_t baseVertex = parentMesh->vertices.size();

    for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;

        for (int i = 0; i < 4; i++) {
            vertex.boneIDs[i] = -1;
            vertex.weights[i] = 0.0f;
        }

        vertex.position = vec4(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z, 1.0f);
        // vertex.normal = glm::normalize(glm::vec4(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z, 0.0f));
        vertex.normal = vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        vertex.texCoord = vec2(0.0f, 0.0f);
        vertex.tangent = vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);

        parentMesh->min = min(parentMesh->min, vertex.position);
        parentMesh->max = max(parentMesh->max, vertex.position);

        if (mesh->mTextureCoords[0]) {
            vertex.texCoord.x = mesh->mTextureCoords[0][i].x;
            vertex.texCoord.y = mesh->mTextureCoords[0][i].y;
        }

        parentMesh->vertices.push_back(vertex);
    }

    subMesh.indexOffset = parentMesh->indices.size();

    for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];

        for (uint32_t j = 0; j < face.mNumIndices; j++) {
            parentMesh->indices.push_back(face.mIndices[j] + baseVertex);
        }
    }

    subMesh.indexCount = parentMesh->indices.size() - subMesh.indexOffset;

    aiColor4D baseColor(1.0f, 1.0f, 1.0f, 1.0f);
    Material* newMaterial = nullptr;

    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        std::string name = material->GetName().C_Str();

        if (gameScene->materialMap.count(name)) {
            newMaterial = gameScene->materialMap[name];
        } else {
            newMaterial = new Material();
            Texture albedoTexture = loadTexture(gameScene, material, aiTextureType_DIFFUSE, directory, allTextures, whiteIsDefault, true);
            Texture roughnessTexture = loadTexture(gameScene, material, aiTextureType_METALNESS, directory, allTextures, whiteIsDefault, true);
            Texture metallicTexture = loadTexture(gameScene, material, aiTextureType_DIFFUSE_ROUGHNESS, directory, allTextures, whiteIsDefault, true);
            Texture aoTexture = loadTexture(gameScene, material, aiTextureType_AMBIENT_OCCLUSION, directory, allTextures, whiteIsDefault, true);
            Texture normalTexture = loadTexture(gameScene, material, aiTextureType_NORMALS, directory, allTextures, whiteIsDefault, false);

            material->Get(AI_MATKEY_COLOR_DIFFUSE, baseColor);

            newMaterial->shader = shader;
            newMaterial->baseColor = vec4(baseColor.r, baseColor.g, baseColor.b, baseColor.a);
            newMaterial->textures.push_back(albedoTexture);
            newMaterial->textures.push_back(roughnessTexture);
            newMaterial->textures.push_back(metallicTexture);
            newMaterial->textures.push_back(aoTexture);
            newMaterial->textures.push_back(normalTexture);
            newMaterial->name = material->GetName().C_Str();
            gameScene->materialMap[name] = newMaterial;
        }
    } else {
        newMaterial = gameScene->materialMap["default"];
    }

    subMesh.material = newMaterial;
    parentMesh->subMeshes.push_back(subMesh);
}

JPH::Mat44 transpose(const aiMatrix4x4& m) {
    return JPH::Mat44(
        JPH::Vec4(m.a1, m.b1, m.c1, m.d1),
        JPH::Vec4(m.a2, m.b2, m.c2, m.d2),
        JPH::Vec4(m.a3, m.b3, m.c3, m.d3),
        JPH::Vec4(m.a4, m.b4, m.c4, m.d4));
}

ModelNode* processNode(aiNode* node, const aiScene* scene, Scene* gameScene, mat4 parentTransform, Model* model, ModelNode* parentNode, std::string* directory, std::vector<Texture>* allTextures, GLuint shader, bool whiteIsDefault) {
    mat4 nodeTransform = transpose(node->mTransformation);
    mat4 globalTransform = parentTransform * nodeTransform;
    ModelNode* childNode = new ModelNode();
    childNode->transform = globalTransform;
    childNode->localTransform = nodeTransform;
    childNode->parent = parentNode;
    childNode->name = node->mName.C_Str();
    childNode->mesh = nullptr;

    for (Animation* animation : model->animations) {
        for (AnimationChannel* channel : animation->channels) {
            if (node->mName.C_Str() == channel->name) {
                model->channelMap[childNode] = channel;

                /* for (int i = 0; i < channel->positions.size(); i++) {
                    channel->positions[i].position = childNode->transform * glm::vec4(channel->positions[i].position, 1.0f);
                }

                for (int i = 0; i < channel->rotations.size(); i++) {
                    glm::quat nodeRotation = quatFromMatrix(childNode->transform);
                    channel->rotations[i].rotation = nodeRotation * channel->rotations[i].rotation;
                }

                for (int i = 0; i < channel->scales.size(); i++) {
                    glm::vec3 nodeScale = scaleFromMatrix(childNode->transform);
                    channel->scales[i].scale = channel->scales[i].scale / nodeScale;
                } */
            }
        }
    }
    if (node->mNumMeshes != 0) {
        childNode->mesh = new Mesh();
        childNode->mesh->min = vec3(scene->mMeshes[node->mMeshes[0]]->mVertices[0].x, scene->mMeshes[node->mMeshes[0]]->mVertices[0].y, scene->mMeshes[node->mMeshes[0]]->mVertices[0].z);
        childNode->mesh->max = childNode->mesh->min;
        childNode->mesh->name = node->mName.C_Str();
        childNode->mesh->globalInverseTransform = model->RootNodeTransform.Inversed();
        childNode->mesh->globalInverseTransform = childNode->transform;
        for (uint32_t i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

            processSubMesh(gameScene, mesh, scene, childNode->mesh, globalTransform, directory, allTextures, shader, whiteIsDefault);

            if (mesh->mNumBones == 0) {
                for (Vertex& vertex : childNode->mesh->vertices) {
                    for (uint32_t i = 0; i < 4; i++) {
                        vertex.boneIDs[i] = 200;
                    }
                }
            }

            for (uint32_t k = 0; k < mesh->mNumBones; k++) {
                BoneInfo bone;
                auto assimpBone = mesh->mBones[k];
                bone.id = k;
                bone.offset = transpose(assimpBone->mOffsetMatrix);

                // bone.offset = childNode->transform * bone.offset;
                childNode->mesh->boneNameMap[assimpBone->mName.C_Str()] = bone;

                auto weights = assimpBone->mWeights;
                uint32_t numWeights = assimpBone->mNumWeights;

                for (uint32_t l = 0; l < numWeights; l++) {
                    uint32_t vertexID = weights[l].mVertexId;
                    float weight = weights[l].mWeight;
                    Vertex& vertex = childNode->mesh->vertices[vertexID];

                    for (uint32_t m = 0; m < 4; m++) {
                        if (vertex.boneIDs[m] < 0) {
                            vertex.weights[m] = weight;
                            vertex.boneIDs[m] = bone.id;
                            break;
                        }
                    }
                }
            }
        }

        childNode->mesh->center = (childNode->mesh->min + childNode->mesh->max) * 0.5f;
        childNode->mesh->extent = (childNode->mesh->max - childNode->mesh->min) * 0.5f;

        model->meshes.push_back(childNode->mesh);

        for (int i = 0; i < childNode->mesh->subMeshes.size(); i++) {
            model->materials.push_back(childNode->mesh->subMeshes[i].material);
        }

        createMeshBuffers(childNode->mesh);
        gameScene->meshMap[childNode->mesh->name] = childNode->mesh;
    }

    for (uint32_t i = 0; i < node->mNumChildren; i++) {
        childNode->children.push_back(processNode(node->mChildren[i], scene, gameScene, globalTransform, model, childNode, directory, allTextures, shader, whiteIsDefault));
    }

    return childNode;
}

Model* loadModel(Scene* gameScene, std::string path, std::vector<Texture>* allTextures, GLuint shader, bool whiteIsDefault) {
    Assimp::Importer importer;
    std::string directory;

    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return nullptr;
    }

    directory = path.substr(0, path.find_last_of('/'));
    std::string name = scene->mRootNode->mName.C_Str();
    name = name.substr(0, name.find_last_of('.'));

    Model* newModel = new Model();
    newModel->name = name;
    processAnimations(gameScene, scene, newModel);
    newModel->RootNodeTransform = transpose(scene->mRootNode->mTransformation);
    newModel->rootNode = processNode(scene->mRootNode, scene, gameScene, mat4::sIdentity(), newModel, nullptr, &directory, allTextures, shader, whiteIsDefault);
    return newModel;
}

void createDefaultResources(Scene* scene) {
    GLuint whiteTexture;
    GLuint blackTexture;
    GLuint blueTexture;

    unsigned char whitePixel[4] = {255, 255, 255, 255};
    unsigned char blackPixel[4] = {0, 0, 0, 255};
    unsigned char bluePixel[4] = {0, 0, 255, 255};

    glGenTextures(1, &whiteTexture);
    glGenTextures(1, &blackTexture);
    glGenTextures(1, &blueTexture);

    glBindTexture(GL_TEXTURE_2D, whiteTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
    glBindTexture(GL_TEXTURE_2D, blackTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, blackPixel);
    glBindTexture(GL_TEXTURE_2D, blueTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, bluePixel);

    Texture white;
    Texture black;
    Texture blue;

    white.id = whiteTexture;
    black.id = blackTexture;
    blue.id = blueTexture;

    white.path = "white";
    white.name = "white";
    black.path = "black";
    black.name = "black";
    blue.path = "blue";
    blue.name = "blue";

    scene->textures.push_back(black);
    scene->textures.push_back(white);
    scene->textures.push_back(blue);

    scene->textureMap[white.name] = white;
    scene->textureMap[black.name] = black;
    scene->textureMap[blue.name] = blue;

    Material* defaultMaterial = new Material();
    defaultMaterial->textures.push_back(white);
    defaultMaterial->textures.push_back(white);
    defaultMaterial->textures.push_back(black);
    defaultMaterial->textures.push_back(white);
    defaultMaterial->textures.push_back(blue);
    defaultMaterial->baseColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    defaultMaterial->shader = scene->lightingShader;
    defaultMaterial->name = "default";
    scene->materialMap[defaultMaterial->name] = defaultMaterial;
}

void loadResources(Scene* scene) {
    createDefaultResources(scene);
    scene->testRoom = loadModel(scene, "../resources/models/testroom/testroom.gltf", &scene->textures, scene->lightingShader, true);
    scene->trashcanModel = loadModel(scene, "../resources/models/trashcan/trashcan.gltf", &scene->textures, scene->lightingShader, true);
    scene->wrenchArms = loadModel(scene, "../resources/models/Arms/wrencharms.gltf", &scene->textures, scene->lightingShader, true);
}