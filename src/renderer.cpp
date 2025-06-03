#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <random>
#include <iostream>

#include "renderer.h"
#include "scene.h"
#include "transform.h"
#include "shader.h"
#include "ecs.h"
#include "camera.h"
#define M_PI 3.14159265358979323846

static void setInitialFlags() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0, 0.0, 0.0, 0.0);
}

void drawPickingScene(RenderState* renderer, EntityGroup* entities) {
    Camera* camera = &entities->cameras[0];
    MeshRenderer* meshRenderer;
    Mesh* mesh;
    SubMesh* subMesh;

    glBindFramebuffer(GL_FRAMEBUFFER, renderer->pickingFBO);
    glViewport(0, 0, renderer->windowData.viewportWidth, renderer->windowData.viewportHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(renderer->pickingShader);

    for (int i = 0; i < entities->meshRenderers.size(); i++) {
        meshRenderer = &entities->meshRenderers[i];
        mesh = meshRenderer->mesh;
        if (mesh == nullptr) {
            continue;
        }

        Transform* transform = getTransform(entities, meshRenderer->entityID);

        mat4 model = transform->worldTransform;
        glBindVertexArray(mesh->VAO);
        unsigned char r = meshRenderer->entityID & 0xFF;
        unsigned char g = (meshRenderer->entityID >> 8) & 0xFF;
        unsigned char b = (meshRenderer->entityID >> 16) & 0xFF;
        vec3 idColor = vec3(r, g, b) / 255.0f;
        glUniformMatrix4fv(uniform_location::kModelMatrix, 1, GL_FALSE, &model(0, 0));
        glUniform3fv(uniform_location::kColor, 1, idColor.mF32);

        for (int i = 0; i < mesh->subMeshes.size(); i++) {
            subMesh = &mesh->subMeshes[i];
            glDrawElements(GL_TRIANGLES, subMesh->indexCount, GL_UNSIGNED_INT, (void*)(subMesh->indexOffset * sizeof(unsigned int)));
        }
    }
}

static void drawShadowMaps(RenderState* renderer, EntityGroup* entities) {
    vec3 position;
    mat4 viewMatrix;
    mat4 projectionMatrix;
    mat4 viewProjection;
    mat4 model;
    GLint boneMatrixLoc = glGetUniformLocation(renderer->depthShader, "finalBoneMatrices[0]");
    MeshRenderer* meshRenderer;
    Mesh* mesh;
    SubMesh* subMesh;

    for (int i = 0; i < entities->spotLights.size(); i++) {
        SpotLight* light = &entities->spotLights[i];
        if (!light->enableShadows || !light->isActive) {
            continue;
        }

        position = getPosition(entities, light->entityID);
        viewMatrix = mat4::sLookAt(position, position + transformForward(entities, light->entityID), transformUp(entities, light->entityID));
        projectionMatrix = mat4::sPerspective(JPH::DegreesToRadians(light->outerCutoff) * 2.0f, 1.0f, 2.1f, light->range);
        viewProjection = projectionMatrix * viewMatrix;
        light->lightSpaceMatrix = viewProjection;

        glViewport(0, 0, light->shadowWidth, light->shadowHeight);
        glBindFramebuffer(GL_FRAMEBUFFER, light->depthFrameBuffer);
        glClear(GL_DEPTH_BUFFER_BIT);

        glUseProgram(renderer->depthShader);
        glUniformMatrix4fv(1, 1, GL_FALSE, &viewProjection(0, 0));
        glUniform3fv(glGetUniformLocation(renderer->depthShader, "lightPos"), 1, position.mF32);
        glUniform1f(glGetUniformLocation(renderer->depthShader, "farPlane"), 200.0f);

        for (int i = 0; i < entities->meshRenderers.size(); i++) {
            meshRenderer = &entities->meshRenderers[i];
            mesh = meshRenderer->mesh;
            if (mesh == nullptr) {
                continue;
            }

            model = getTransform(entities, meshRenderer->entityID)->worldTransform;
            glUniformMatrix4fv(2, 1, GL_FALSE, &model(0, 0));

            if (!meshRenderer->boneMatricesSet && meshRenderer->boneMatrices.size() > 0) {
                Transform* boneTransform;
                uint32_t index;
                mat4 offset;
                meshRenderer->boneMatricesSet = true;

                for (const auto& pair : meshRenderer->transformBoneMap) {
                    boneTransform = getTransform(entities, pair.first);
                    index = pair.second.id;
                    offset = pair.second.offset;
                    meshRenderer->boneMatrices[index] = (getTransform(entities, meshRenderer->rootEntity)->worldTransform).Inversed() * boneTransform->worldTransform * offset;
                }

                glUniformMatrix4fv(boneMatrixLoc, meshRenderer->boneMatrices.size(), GL_FALSE, &meshRenderer->boneMatrices[0](0, 0));
            }

            glBindVertexArray(mesh->VAO);

            for (int i = 0; i < mesh->subMeshes.size(); i++) {
                subMesh = &mesh->subMeshes[i];
                glDrawElements(GL_TRIANGLES, subMesh->indexCount, GL_UNSIGNED_INT, (void*)(subMesh->indexOffset * sizeof(GLsizei)));
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, light->blurDepthFrameBuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(renderer->shadowBlurShader);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, light->depthTex);
        glBindVertexArray(renderer->fullscreenVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    glViewport(0, 0, renderer->windowData.viewportWidth, renderer->windowData.viewportHeight);
}

static void drawScene(RenderState* renderer, EntityGroup* entities) {
    uint32_t offset;
    mat4 model;
    MeshRenderer* meshRenderer;
    Mesh* mesh;
    SubMesh* subMesh;
    Material* material;
    PointLight* pointLight;
    SpotLight* spotLight;
    Camera* camera = &entities->cameras[0];

    glBindFramebuffer(GL_FRAMEBUFFER, renderer->litFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(renderer->lightingShader);
    glUniform3fv(8, 1, getLocalPosition(entities, camera->entityID).mF32);
    glUniform1f(9, renderer->bloomThreshold);
    glUniform1f(35, renderer->ambient);
    glUniform1i(6, entities->spotLights.size());
    glUniform1i(7, entities->pointLights.size());
    glUniform1f(glGetUniformLocation(renderer->lightingShader, "fogDensity"), renderer->fogDensity);
    glUniform1f(glGetUniformLocation(renderer->lightingShader, "minFogDistance"), renderer->minFogDistance);
    glUniform1f(glGetUniformLocation(renderer->lightingShader, "maxFogDistance"), renderer->maxFogDistance);
    glUniform3fv(glGetUniformLocation(renderer->lightingShader, "fogColor"), 1, renderer->fogColor.mF32);

    for (uint32_t i = 0; i < entities->pointLights.size(); i++) {
        pointLight = &entities->pointLights[i];
        offset = i * 4;
        glUniform3fv(36 + 0 + offset, 1, getPosition(entities, pointLight->entityID).mF32);
        glUniform3fv(36 + 1 + offset, 1, pointLight->color.mF32);
        glUniform1f(36 + 2 + offset, pointLight->brightness);
    }

    for (uint32_t i = 0; i < entities->spotLights.size(); i++) {
        spotLight = &entities->spotLights[i];
        offset = i * 8;
        glUniform3fv(120 + 0 + offset, 1, getPosition(entities, spotLight->entityID).mF32);
        glUniform3fv(120 + 1 + offset, 1, transformForward(entities, spotLight->entityID).mF32);
        glUniform3fv(120 + 2 + offset, 1, spotLight->color.mF32);
        glUniform1f(120 + 3 + offset, spotLight->brightness);
        glUniform1f(120 + 4 + offset, JPH::Cos(JPH::DegreesToRadians(spotLight->cutoff)));
        glUniform1f(120 + 5 + offset, JPH::Cos(JPH::DegreesToRadians(spotLight->outerCutoff)));
        glUniform1i(120 + 6 + offset, spotLight->isActive);
        glUniform1i(120 + 7 + offset, spotLight->enableShadows);
        glUniformMatrix4fv(15 + i, 1, GL_FALSE, &spotLight->lightSpaceMatrix(0, 0));
        glUniform1f(glGetUniformLocation(renderer->lightingShader, "u_LightRadiusUV"), spotLight->lightRadiusUV);
        glUniform1f(glGetUniformLocation(renderer->lightingShader, "u_BlockerSearchUV"), spotLight->blockerSearchUV);
        glActiveTexture(GL_TEXTURE0 + uniform_location::kTextureShadowMapUnit + i);
        glBindTexture(GL_TEXTURE_2D, spotLight->blurDepthTex);
    }

    for (int i = 0; i < entities->meshRenderers.size(); i++) {
        meshRenderer = &entities->meshRenderers[i];
        mesh = meshRenderer->mesh;
        if (mesh == nullptr) {
            continue;
        }

        if (meshRenderer->boneMatrices.size() > 0) {
            if (!meshRenderer->boneMatricesSet) {
                Transform* boneTransform;
                uint32_t index;
                mat4 offset;
                meshRenderer->boneMatricesSet = true;

                for (const auto& pair : meshRenderer->transformBoneMap) {
                    boneTransform = getTransform(entities, pair.first);
                    index = pair.second.id;
                    offset = pair.second.offset;
                    meshRenderer->boneMatrices[index] = (getTransform(entities, meshRenderer->rootEntity)->worldTransform).Inversed() * boneTransform->worldTransform * offset;
                }
            }
            // glUniformMatrix4fv(boneMatrixLoc, meshRenderer.boneMatrices.size(), GL_FALSE, &meshRenderer.boneMatrices[0](0, 0));
            meshRenderer->boneMatricesSet = false;
            glUniformMatrix4fv(glGetUniformLocation(renderer->lightingShader, "finalBoneMatrices[0]"), meshRenderer->boneMatrices.size(), GL_FALSE, &meshRenderer->boneMatrices[0](0, 0));
        }

        model = getTransform(entities, meshRenderer->entityID)->worldTransform;
        glUniformMatrix4fv(4, 1, GL_FALSE, &model(0, 0));
        glBindVertexArray(mesh->VAO);

        for (int i = 0; i < mesh->subMeshes.size(); i++) {
            subMesh = &mesh->subMeshes[i];
            material = subMesh->material;
            const std::vector<Texture*>& textures = material->textures;

            glUniform1f(10, material->metalness);
            glUniform1f(11, material->roughness);
            glUniform1f(12, material->aoStrength);
            glUniform1f(13, material->normalStrength);

#ifdef PETES_EDITOR
            // vec4 baseColor = scene->pickedEntity == meshRenderer->entityID ? vec4(0.0f, 1.0f, 0.0f, 1.0f) : material->baseColor;
            vec4 baseColor = material->baseColor;
            glUniform3fv(14, 1, baseColor.mF32);
#else
            glUniform3fv(14, 1, material->baseColor.mF32);
#endif
            glUniform2fv(glGetUniformLocation(renderer->lightingShader, "textureTiling"), 1, glm::value_ptr(material->textureTiling));

            glActiveTexture(GL_TEXTURE0 + uniform_location::kTextureAlbedoUnit);
            glBindTexture(GL_TEXTURE_2D, textures[0]->id);
            glActiveTexture(GL_TEXTURE0 + uniform_location::kTextureRoughnessUnit);
            glBindTexture(GL_TEXTURE_2D, textures[1]->id);
            glActiveTexture(GL_TEXTURE0 + uniform_location::kTextureMetallicUnit);
            glBindTexture(GL_TEXTURE_2D, textures[2]->id);
            glActiveTexture(GL_TEXTURE0 + uniform_location::kTextureAOUnit);
            glBindTexture(GL_TEXTURE_2D, textures[3]->id);
            glActiveTexture(GL_TEXTURE0 + uniform_location::kTextureNormalUnit);
            glBindTexture(GL_TEXTURE_2D, textures[4]->id);

            glDrawElements(GL_TRIANGLES, subMesh->indexCount, GL_UNSIGNED_INT, (void*)(subMesh->indexOffset * sizeof(unsigned int)));
        }
    }
}

static void drawSSAO(RenderState* renderer) {
    glBindFramebuffer(GL_FRAMEBUFFER, renderer->ssaoFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(renderer->ssaoShader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->ssaoPosTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, renderer->ssaoNormalTex);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, renderer->ssaoNoiseTex);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, renderer->bloomSSAOTex);

    glUniform1f(6, renderer->AORadius);
    glUniform1f(7, renderer->AOBias);
    glUniform1f(9, renderer->AOPower);

    glBindVertexArray(renderer->fullscreenVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

static void drawBlurPass(RenderState* renderer) {
    bool horizontalBlur = false;
    const uint32_t amount = 10;
    glUseProgram(renderer->blurShader);
    glActiveTexture(GL_TEXTURE0);
    glBindFramebuffer(GL_FRAMEBUFFER, renderer->blurFBO[1]);
    glUniform1i(uniform_location::kBHorizontal, true);
    glBindTexture(GL_TEXTURE_2D, renderer->blurTex);
    glBindVertexArray(renderer->fullscreenVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    for (uint32_t i = 0; i < amount - 1; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, renderer->blurFBO[horizontalBlur]);
        glUniform1i(uniform_location::kBHorizontal, horizontalBlur);
        glBindTexture(GL_TEXTURE_2D, renderer->blurSwapTex[!horizontalBlur]);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        horizontalBlur = !horizontalBlur;
    }
}

void MyDebugRenderer::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) {
    lines.push_back({inFrom, inTo, inColor});
}

void MyDebugRenderer::DrawTriangle(JPH::RVec3Arg inV1, const JPH::RVec3Arg inV2, const JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow) {
    triangles.push_back({inV1, inV2, inV3, inColor});
}

void MyDebugRenderer::DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight) {
    // Implement
}

void MyDebugRenderer::Clear() {
    lines.clear();
    triangles.clear();
}

void renderDebug(RenderState* renderer) {
    glDisable(GL_DEPTH_TEST);
    glUseProgram(renderer->debugShader);
    MyDebugRenderer* debug = static_cast<MyDebugRenderer*>(renderer->debugRenderer);
    std::vector<DebugVertex> lineVerts;

    for (DebugLine& line : debug->lines) {
        lineVerts.push_back({line.start, line.color.ToVec4()});
        lineVerts.push_back({line.end, line.color.ToVec4()});
    }

    GLuint lineVBO, lineVAO;
    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, lineVerts.size() * sizeof(DebugVertex), lineVerts.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DebugVertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(DebugVertex), (void*)offsetof(DebugVertex, color));
    glEnableVertexAttribArray(1);

    glDrawArrays(GL_LINES, 0, lineVerts.size());

    glDeleteBuffers(1, &lineVBO);
    glDeleteVertexArrays(1, &lineVAO);

    // Triangles
    std::vector<DebugVertex> triVerts;
    for (DebugTri& tri : debug->triangles) {
        triVerts.push_back({tri.v0, tri.color.ToVec4()});
        triVerts.push_back({tri.v1, tri.color.ToVec4()});
        triVerts.push_back({tri.v2, tri.color.ToVec4()});
    }

    GLuint triVBO, triVAO;
    glGenVertexArrays(1, &triVAO);
    glGenBuffers(1, &triVBO);
    glBindVertexArray(triVAO);
    glBindBuffer(GL_ARRAY_BUFFER, triVBO);
    glBufferData(GL_ARRAY_BUFFER, triVerts.size() * sizeof(DebugVertex), triVerts.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DebugVertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(DebugVertex), (void*)offsetof(DebugVertex, color));
    glEnableVertexAttribArray(1);

    glDrawArrays(GL_TRIANGLES, 0, triVerts.size());

    glDeleteBuffers(1, &triVBO);
    glDeleteVertexArrays(1, &triVAO);

    debug->Clear();
    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void drawFullScreenQuad(RenderState* renderer) {
    glViewport(0, 0, renderer->windowData.viewportWidth, renderer->windowData.viewportHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, renderer->finalBuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(renderer->postProcessShader);
    glUniform1f(uniform_location::kPExposure, renderer->exposure);
    glUniform1f(uniform_location::kPBloomAmount, renderer->bloomAmount);
    glUniform1f(uniform_location::kAOAmount, renderer->AOAmount);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->litColorTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, renderer->blurSwapTex[1]);
    glBindVertexArray(renderer->fullscreenVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

static void createPickingFBO(RenderState* renderer) {
    GLsizei width = renderer->windowData.width;
    GLsizei height = renderer->windowData.height;

    glGenFramebuffers(1, &renderer->pickingFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, renderer->pickingFBO);

    glGenTextures(1, &renderer->pickingTex);
    glBindTexture(GL_TEXTURE_2D, renderer->pickingTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderer->pickingTex, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenRenderbuffers(1, &renderer->pickingRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, renderer->pickingRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderer->pickingRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void createSpotLightShadowMap(SpotLight* light) {
    light->shadowWidth = 1024.0f;
    light->shadowHeight = 1024.0f;
    glGenFramebuffers(1, &light->depthFrameBuffer);
    glGenTextures(1, &light->depthTex);
    glBindTexture(GL_TEXTURE_2D, light->depthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, light->shadowWidth, light->shadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1.0, 1.0, 1.0, 1.0};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindFramebuffer(GL_FRAMEBUFFER, light->depthFrameBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, light->depthTex, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER:: Depth framebuffer is not complete!" << std::endl;
    }

    glGenFramebuffers(1, &light->blurDepthFrameBuffer);
    glGenTextures(1, &light->blurDepthTex);
    glBindFramebuffer(GL_FRAMEBUFFER, light->blurDepthFrameBuffer);
    glBindTexture(GL_TEXTURE_2D, light->blurDepthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, light->shadowWidth, light->shadowHeight, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, light->blurDepthTex, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER:: Depth framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void deleteSpotLightShadowMap(SpotLight* light) {
    // this may break if the textures/framebuffers haven't been created in the case of a prefab. we'll see.
    GLuint textures[2] = {light->depthTex, light->blurDepthTex};
    GLuint frameBuffers[2] = {light->depthFrameBuffer, light->blurDepthFrameBuffer};
    glDeleteTextures(2, textures);
    glDeleteFramebuffers(2, frameBuffers);
}

static void createShadowMapDepthBuffers(EntityGroup* entities) {  // this needs to just be callable per light, not looping over every light.
    for (SpotLight& light : entities->spotLights) {
        light.shadowWidth = 512.0f;
        light.shadowHeight = 512.0f;
        glGenFramebuffers(1, &light.depthFrameBuffer);
        glGenTextures(1, &light.depthTex);
        glBindTexture(GL_TEXTURE_2D, light.depthTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, light.shadowWidth, light.shadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = {1.0, 1.0, 1.0, 1.0};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        glBindFramebuffer(GL_FRAMEBUFFER, light.depthFrameBuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, light.depthTex, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "ERROR::FRAMEBUFFER:: Depth framebuffer is not complete!" << std::endl;
        }

        glGenFramebuffers(1, &light.blurDepthFrameBuffer);
        glGenTextures(1, &light.blurDepthTex);
        glBindFramebuffer(GL_FRAMEBUFFER, light.blurDepthFrameBuffer);
        glBindTexture(GL_TEXTURE_2D, light.blurDepthTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, light.shadowWidth, light.shadowHeight, 0, GL_RED, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, light.blurDepthTex, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "ERROR::FRAMEBUFFER:: Depth framebuffer is not complete!" << std::endl;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

static void createForwardBuffer(RenderState* renderer) {
    GLsizei width = renderer->windowData.viewportWidth;
    GLsizei height = renderer->windowData.viewportHeight;
    GLenum attachments[4] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};

    glGenFramebuffers(1, &renderer->litFBO);
    glGenTextures(1, &renderer->litColorTex);
    glGenTextures(1, &renderer->bloomSSAOTex);
    glGenTextures(1, &renderer->ssaoNormalTex);
    glGenTextures(1, &renderer->ssaoPosTex);
    glGenRenderbuffers(1, &renderer->litRBO);

    glBindFramebuffer(GL_FRAMEBUFFER, renderer->litFBO);

    glBindTexture(GL_TEXTURE_2D, renderer->litColorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, renderer->bloomSSAOTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, renderer->ssaoPosTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // normal color buffer
    glBindTexture(GL_TEXTURE_2D, renderer->ssaoNormalTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindRenderbuffer(GL_RENDERBUFFER, renderer->litRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderer->litColorTex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, renderer->bloomSSAOTex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, renderer->ssaoPosTex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, renderer->ssaoNormalTex, 0);
    glDrawBuffers(4, attachments);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderer->litRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER:: GBuffer framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void createEditorBuffer(RenderState* renderer) {
    GLint width = renderer->windowData.viewportWidth;
    GLint height = renderer->windowData.viewportHeight;

    glGenFramebuffers(1, &renderer->editorFBO);
    glGenTextures(1, &renderer->editorTex);
    glGenRenderbuffers(1, &renderer->editorRBO);

    glBindFramebuffer(GL_FRAMEBUFFER, renderer->editorFBO);
    glBindTexture(GL_TEXTURE_2D, renderer->editorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderer->editorTex, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, renderer->editorRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderer->editorRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER::Editor framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
static void createSSAOBuffer(RenderState* renderer) {
    glGenFramebuffers(1, &renderer->ssaoFBO);
    glGenTextures(1, &renderer->blurTex);

    glBindFramebuffer(GL_FRAMEBUFFER, renderer->ssaoFBO);
    glBindTexture(GL_TEXTURE_2D, renderer->blurTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, renderer->windowData.viewportWidth, renderer->windowData.viewportHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderer->blurTex, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER:: SSAO Color framebuffer is not complete!" << std::endl;
    }
}

static void createBlurBuffers(RenderState* renderer) {
    GLsizei width = renderer->windowData.viewportWidth;
    GLsizei height = renderer->windowData.viewportHeight;

    glGenFramebuffers(2, renderer->blurFBO);
    glGenTextures(2, renderer->blurSwapTex);

    for (unsigned int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, renderer->blurFBO[i]);
        glBindTexture(GL_TEXTURE_2D, renderer->blurSwapTex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderer->blurSwapTex[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "ERROR::FRAMEBUFFER:: Blur framebuffer is not complete!" << std::endl;
        }
    }
}

static void resizeBuffers(RenderState* renderer) {
    uint32_t width = renderer->windowData.viewportWidth;
    uint32_t height = renderer->windowData.viewportHeight;
    GLenum attachments[4] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};

    glBindFramebuffer(GL_FRAMEBUFFER, renderer->litFBO);
    glBindTexture(GL_TEXTURE_2D, renderer->litColorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, renderer->bloomSSAOTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, renderer->ssaoPosTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, renderer->ssaoNormalTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindRenderbuffer(GL_RENDERBUFFER, renderer->litRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderer->litColorTex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, renderer->bloomSSAOTex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, renderer->ssaoPosTex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, renderer->ssaoNormalTex, 0);

    glDrawBuffers(4, attachments);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderer->litRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER:: GBuffer framebuffer is not complete!" << std::endl;
    }

    for (unsigned int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, renderer->blurFBO[i]);
        glBindTexture(GL_TEXTURE_2D, renderer->blurSwapTex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderer->blurSwapTex[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "ERROR::FRAMEBUFFER:: Blur framebuffer is not complete!" << std::endl;
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, renderer->pickingFBO);
    glBindTexture(GL_TEXTURE_2D, renderer->pickingTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderer->pickingTex, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, renderer->pickingRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderer->pickingRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER:: Depth framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, renderer->ssaoFBO);
    glBindTexture(GL_TEXTURE_2D, renderer->blurTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, renderer->windowData.viewportWidth, renderer->windowData.viewportHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderer->blurTex, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER:: SSAO Color framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, renderer->editorFBO);
    glBindTexture(GL_TEXTURE_2D, renderer->editorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderer->editorTex, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, renderer->editorRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderer->editorRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER::Editor framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void createFullScreenQuad(RenderState* renderer) {
    float quadVertices[] = {
        -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,   // top-left
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,  // bottom-left
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f,    // top-right
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f,   // bottom-right
    };
    glGenVertexArrays(1, &renderer->fullscreenVAO);
    glGenBuffers(1, &renderer->fullscreenVBO);

    glBindVertexArray(renderer->fullscreenVAO);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->fullscreenVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(vertex_attribute_location::kVertexPosition);
    glVertexAttribPointer(vertex_attribute_location::kVertexPosition, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(vertex_attribute_location::kVertexTexCoord);
    glVertexAttribPointer(vertex_attribute_location::kVertexTexCoord, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
}

static void generateSSAOKernel(RenderState* renderer) {
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
    std::default_random_engine generator;

    renderer->ssaoKernel.clear();
    renderer->ssaoNoise.clear();

    for (unsigned int i = 0; i < 8; i++) {
        // Cosine-weighted hemisphere sampling
        float xi1 = randomFloats(generator);
        float xi2 = randomFloats(generator);

        float theta = acos(sqrt(1.0f - xi1));  // inverse CDF for cosine-weighted
        float phi = 2.0f * M_PI * xi2;

        vec3 sample;
        sample.SetX(sin(theta) * cos(phi));
        sample.SetY(sin(theta) * sin(phi));
        sample.SetZ(cos(theta));

        // Apply random scaling to bring samples closer to origin
        float scale = static_cast<float>(i) / 8.0f;
        // Stronger bias toward center → scale^3 instead of scale^2
        scale = lerp(0.05f, 1.0f, scale * scale * scale);
        sample *= scale;

        renderer->ssaoKernel.push_back(sample);
    }

    // noise texture (same as before)
    for (unsigned int i = 0; i < 16; i++) {
        vec3 noise(randomFloats(generator) * 2.0f - 1.0f,
                   randomFloats(generator) * 2.0f - 1.0f,
                   0.0f);  // rotate around z-axis (in tangent space)
        renderer->ssaoNoise.push_back(noise);
    }

    glGenTextures(1, &renderer->ssaoNoiseTex);
    glBindTexture(GL_TEXTURE_2D, renderer->ssaoNoiseTex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGB, GL_FLOAT, &renderer->ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glUseProgram(renderer->ssaoShader);
    for (unsigned int i = 0; i < 8; i++) {
        glUniform3fv(glGetUniformLocation(renderer->ssaoShader, ("samples[" + std::to_string(i) + "]").c_str()), 1, renderer->ssaoKernel[i].mF32);
    }
}

void deleteBuffers(RenderState* renderer, Resources* resources) {
    unsigned int textures[5] = {renderer->pickingTex, renderer->litColorTex, renderer->bloomSSAOTex, renderer->blurTex, renderer->ssaoNoiseTex};
    unsigned int rbo[2] = {renderer->pickingRBO, renderer->litRBO};
    unsigned int frameBuffers[3] = {renderer->pickingFBO, renderer->ssaoFBO, renderer->litFBO};

    glDeleteTextures(5, textures);
    glDeleteRenderbuffers(2, rbo);
    glDeleteFramebuffers(3, frameBuffers);
    glDeleteShader(renderer->lightingShader);
    glDeleteShader(renderer->pickingShader);
    glDeleteShader(renderer->postProcessShader);
    glDeleteShader(renderer->blurShader);
    glDeleteShader(renderer->depthShader);
    glDeleteShader(renderer->ssaoShader);

    glDeleteBuffers(1, &renderer->fullscreenVBO);
    glDeleteVertexArrays(1, &renderer->fullscreenVAO);
    for (auto& pair : resources->textureMap) {
        glDeleteTextures(1, &pair.second->id);
    }

    for (auto pair : resources->meshMap) {
        Mesh* mesh = pair.second;
        glDeleteBuffers(1, &mesh->EBO);
        glDeleteBuffers(1, &mesh->VBO);
        glDeleteVertexArrays(1, &mesh->VAO);
    }
}

void initializeEnvironment(RenderState* renderer, Scene* scene, unsigned int shader) {
    float test[2];
    test[0] = renderer->windowData.viewportWidth / 4.0f;
    test[1] = renderer->windowData.viewportHeight / 4.0f;

    glUseProgram(renderer->ssaoShader);
    glUniform2fv(8, 1, test);

    renderer->AORadius = 0.06f;
    renderer->AOBias = 0.04f;
    renderer->AOPower = 2.02f;
}

void updateBufferData(RenderState* renderer, Scene* scene) {
    EntityGroup* entities = &scene->entities;
    Camera* camera = &entities->cameras[0];
    vec3 position = getPosition(entities, camera->entityID);

    renderer->matricesUBOData.view = mat4::sLookAt(position, position + transformForward(entities, camera->entityID), transformUp(entities, camera->entityID));
    renderer->matricesUBOData.projection = mat4::sPerspective(camera->fovRadians, renderer->windowData.aspectRatio, camera->nearPlane, camera->farPlane);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(GlobalUBO), &renderer->matricesUBOData);
}

void renderScene(RenderState* renderer, EntityGroup* entities) {
    drawShadowMaps(renderer, entities);
    drawScene(renderer, entities);
    drawSSAO(renderer);
    drawBlurPass(renderer);
    drawFullScreenQuad(renderer);
}

void findBones(Scene* scene, MeshRenderer* renderer, Transform* parent) {
    EntityGroup* entities = &scene->entities;
    for (int i = 0; i < parent->childEntityIds.size(); i++) {
        Entity* child = getEntity(entities, parent->childEntityIds[i]);
        if (renderer->mesh->boneNameMap.count(child->name)) {
            renderer->transformBoneMap[child->entityID] = renderer->mesh->boneNameMap[child->name];
        }

        findBones(scene, renderer, getTransform(entities, child->entityID));
    }
}

void mapBones(Scene* scene, MeshRenderer* renderer) {
    if (renderer->mesh->boneNameMap.size() == 0) {
        return;
    }

    EntityGroup* entities = &scene->entities;
    renderer->boneMatrices.reserve(100);

    for (int i = 0; i < 100; i++) {
        renderer->boneMatrices.push_back(mat4::sIdentity());
    }

    Transform* parent = getTransform(entities, renderer->entityID);

    if (parent->parentEntityID != INVALID_ID) {
        parent = getTransform(entities, parent->parentEntityID);
    }

    findBones(scene, renderer, parent);
}

void createCameraUBO(RenderState* renderer) {
    glGenBuffers(1, &renderer->matricesUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, renderer->matricesUBO);
    glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(mat4), NULL, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, renderer->matricesUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, renderer->matricesUBO);
}

void initRenderer(RenderState* renderer, Scene* scene) {
    setInitialFlags();
    createSSAOBuffer(renderer);
    createForwardBuffer(renderer);
    createBlurBuffers(renderer);
    createFullScreenQuad(renderer);
    generateSSAOKernel(renderer);
    createCameraUBO(renderer);
    initializeEnvironment(renderer, scene, renderer->lightingShader);
}

void initRendererEditor(RenderState* renderer) {
    createPickingFBO(renderer);
    createEditorBuffer(renderer);
    renderer->finalBuffer = renderer->editorFBO;
    renderer->debugRenderer = new MyDebugRenderer();
    JPH::DebugRenderer::sInstance = renderer->debugRenderer;
}

void onScreenChanged(GLFWwindow* window, int width, int height) {
    RenderState* renderer = (RenderState*)glfwGetWindowUserPointer(window);
    renderer->windowData.aspectRatio = (float)width / height;
    glViewport(0, 0, width, height);
    glUseProgram(renderer->ssaoShader);
    vec2 v(renderer->windowData.viewportWidth / 4.0f, renderer->windowData.viewportHeight / 4.0f);
    // glUniform2fv(8, 1, &v.x);
    float test[2];
    test[0] = renderer->windowData.viewportWidth / 4.0f;
    test[1] = renderer->windowData.viewportHeight / 4.0f;
    glUniform2fv(8, 1, test);

    resizeBuffers(renderer);
}

void createContext(RenderState* renderer) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwSwapInterval(0);

    renderer->windowData.width = 1920;
    renderer->windowData.height = 1080;
    renderer->windowData.viewportWidth = 1920;
    renderer->windowData.viewportHeight = 1080;
    renderer->window = glfwCreateWindow(renderer->windowData.width, renderer->windowData.height, "Pete's Game", NULL, NULL);

    if (renderer->window == NULL) {
        std::cout << "Failed to create window" << std::endl;
        exit(0);
    }

    glfwMakeContextCurrent(renderer->window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to load GLAD" << std::endl;
        exit(0);
    }

    glfwSetFramebufferSizeCallback(renderer->window, onScreenChanged);
    glfwSetWindowUserPointer(renderer->window, renderer);
}
