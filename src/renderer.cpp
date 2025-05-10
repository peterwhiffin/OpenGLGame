#include <random>
#include "transform.h"
#include "shader.h"
#include "renderer.h"

#define M_PI 3.14159265358979323846

void setFlags() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0, 0.0, 0.0, 0.0);
}

void drawPickingScene(Scene* scene) {
    Camera* camera = scene->cameras[0];

    glBindFramebuffer(GL_FRAMEBUFFER, scene->pickingFBO);
    glViewport(0, 0, scene->windowData.viewportWidth, scene->windowData.viewportHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(scene->pickingShader);
    for (MeshRenderer& renderer : scene->meshRenderers) {
        Transform* transform = getTransform(scene, renderer.entityID);

        glm::mat4 model = transform->worldTransform;
        glBindVertexArray(renderer.mesh->VAO);
        unsigned char r = renderer.entityID & 0xFF;
        unsigned char g = (renderer.entityID >> 8) & 0xFF;
        unsigned char b = (renderer.entityID >> 16) & 0xFF;
        glm::vec3 idColor = glm::vec3(r, g, b) / 255.0f;
        glUniformMatrix4fv(uniform_location::kModelMatrix, 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(uniform_location::kColor, 1, glm::value_ptr(idColor));

        for (SubMesh& subMesh : renderer.mesh->subMeshes) {
            glDrawElements(GL_TRIANGLES, subMesh.indexCount, GL_UNSIGNED_INT, (void*)(subMesh.indexOffset * sizeof(unsigned int)));
        }
    }
}

void drawShadowMaps(Scene* scene) {
    for (SpotLight& light : scene->spotLights) {
        glBindFramebuffer(GL_FRAMEBUFFER, light.depthFrameBuffer);
        glViewport(0, 0, light.shadowWidth, light.shadowHeight);
        glClear(GL_DEPTH_BUFFER_BIT);

        glUseProgram(scene->depthShader);
        glm::vec3 position = getPosition(scene, light.entityID);
        glm::mat4 viewMatrix = glm::lookAt(position, position + forward(scene, light.entityID), up(scene, light.entityID));
        glm::mat4 projectionMatrix = glm::perspective(glm::radians((light.outerCutoff * 2.0f)), (float)light.shadowWidth / light.shadowHeight, 1.1f, 800.0f);
        glm::mat4 viewProjection = projectionMatrix * viewMatrix;
        light.lightSpaceMatrix = viewProjection;
        glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(viewProjection));

        for (MeshRenderer& renderer : scene->meshRenderers) {
            glm::mat4 model = getTransform(scene, renderer.entityID)->worldTransform;
            glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(model));
            glBindVertexArray(renderer.mesh->VAO);

            for (SubMesh& subMesh : renderer.mesh->subMeshes) {
                glDrawElements(GL_TRIANGLES, subMesh.indexCount, GL_UNSIGNED_INT, (void*)(subMesh.indexOffset * sizeof(unsigned int)));
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, light.blurDepthFrameBuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(scene->shadowBlurShader);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, light.depthTex);
        glBindVertexArray(scene->fullscreenVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    glViewport(0, 0, scene->windowData.viewportWidth, scene->windowData.viewportHeight);
}

void drawScene(Scene* scene) {
    Camera* camera = scene->cameras[0];

    glBindFramebuffer(GL_FRAMEBUFFER, scene->litFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(scene->lightingShader);

    for (int i = 0; i < scene->pointLights.size(); i++) {
        PointLight* pointLight = &scene->pointLights[i];
        uint32_t offset = i * 4;
        glUniform3fv(36 + 0 + offset, 1, glm::value_ptr(getPosition(scene, pointLight->entityID)));
        glUniform3fv(36 + 1 + offset, 1, glm::value_ptr(pointLight->color));
        glUniform1f(36 + 2 + offset, pointLight->brightness);
    }

    for (int i = 0; i < scene->spotLights.size(); i++) {
        SpotLight* spotLight = &scene->spotLights[i];
        uint32_t offset = i * 7;
        glUniform3fv(120 + 0 + offset, 1, glm::value_ptr(getPosition(scene, spotLight->entityID)));
        glUniform3fv(120 + 1 + offset, 1, glm::value_ptr(forward(scene, spotLight->entityID)));
        glUniform3fv(120 + 2 + offset, 1, glm::value_ptr(spotLight->color));
        glUniform1f(120 + 3 + offset, spotLight->brightness);
        glUniform1f(120 + 4 + offset, glm::cos(glm::radians(spotLight->cutoff)));
        glUniform1f(120 + 5 + offset, glm::cos(glm::radians(spotLight->outerCutoff)));
        glUniformMatrix4fv(15 + i, 1, GL_FALSE, glm::value_ptr(spotLight->lightSpaceMatrix));
        glActiveTexture(GL_TEXTURE0 + uniform_location::kTextureShadowMapUnit + i);
        glBindTexture(GL_TEXTURE_2D, spotLight->blurDepthTex);
    }

    glUniform3fv(8, 1, glm::value_ptr(getLocalPosition(scene, camera->entityID)));
    glUniform1f(9, scene->bloomThreshold);
    glUniform1f(35, scene->ambient);

    for (MeshRenderer& renderer : scene->meshRenderers) {
        glm::mat4 model = getTransform(scene, renderer.entityID)->worldTransform;
        glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(model));

        if (renderer.boneMatrices.size() > 0) {
            for (auto& pair : renderer.transformBoneMap) {
                Transform* boneTransform = getTransform(scene, pair.first);
                // std::cout << getEntity(scene, pair.first)->name << ": " << pair.second.id << " - " << glm::to_string(getPosition(scene, boneTransform->entityID)) << "\n";
                uint32_t index = pair.second.id;
                glm::mat4 offset = pair.second.offset;

                // renderer.boneMatrices[index] = renderer.mesh->globalInverseTransform * boneTransform->worldTransform * offset;
                renderer.boneMatrices[index] = renderer.mesh->globalInverseTransform * boneTransform->worldTransform * offset;
            }

            glUniformMatrix4fv(glGetUniformLocation(scene->lightingShader, "finalBoneMatrices[0]"), renderer.boneMatrices.size(), GL_FALSE, glm::value_ptr(renderer.boneMatrices[0]));
        }

        glBindVertexArray(renderer.mesh->VAO);

        for (SubMesh& subMesh : renderer.mesh->subMeshes) {
            glUniform1f(10, subMesh.material->metalness);
            glUniform1f(11, subMesh.material->roughness);
            glUniform1f(12, subMesh.material->aoStrength);
            glUniform1f(13, subMesh.material->normalStrength);
            // glm::vec3 baseColor = (renderer->entityID == scene->nodeClicked) ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 1.0f, 1.0f);

            glUniform3fv(14, 1, glm::value_ptr(subMesh.material->baseColor));

            glActiveTexture(GL_TEXTURE0 + uniform_location::kTextureAlbedoUnit);
            glBindTexture(GL_TEXTURE_2D, subMesh.material->textures[0].id);
            glActiveTexture(GL_TEXTURE0 + uniform_location::kTextureRoughnessUnit);
            glBindTexture(GL_TEXTURE_2D, subMesh.material->textures[1].id);
            glActiveTexture(GL_TEXTURE0 + uniform_location::kTextureMetallicUnit);
            glBindTexture(GL_TEXTURE_2D, subMesh.material->textures[2].id);
            glActiveTexture(GL_TEXTURE0 + uniform_location::kTextureAOUnit);
            glBindTexture(GL_TEXTURE_2D, subMesh.material->textures[3].id);
            glActiveTexture(GL_TEXTURE0 + uniform_location::kTextureNormalUnit);
            glBindTexture(GL_TEXTURE_2D, subMesh.material->textures[4].id);

            glDrawElements(GL_TRIANGLES, subMesh.indexCount, GL_UNSIGNED_INT, (void*)(subMesh.indexOffset * sizeof(unsigned int)));
        }
    }
}

void drawSSAO(Scene* scene) {
    glBindFramebuffer(GL_FRAMEBUFFER, scene->ssaoFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(scene->ssaoShader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, scene->ssaoPosTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, scene->ssaoNormalTex);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, scene->ssaoNoiseTex);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, scene->bloomSSAOTex);

    glUniform1f(6, scene->AORadius);
    glUniform1f(7, scene->AOBias);
    glUniform1f(9, scene->AOPower);

    glBindVertexArray(scene->fullscreenVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void drawBlurPass(Scene* scene) {
    scene->horizontalBlur = false;
    int amount = 10;
    glUseProgram(scene->blurShader);
    glActiveTexture(GL_TEXTURE0);
    glBindFramebuffer(GL_FRAMEBUFFER, scene->blurFBO[1]);
    glUniform1i(uniform_location::kBHorizontal, true);
    glBindTexture(GL_TEXTURE_2D, scene->blurTex);
    glBindVertexArray(scene->fullscreenVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    for (unsigned int i = 0; i < amount - 1; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, scene->blurFBO[scene->horizontalBlur]);
        glUniform1i(uniform_location::kBHorizontal, scene->horizontalBlur);
        glBindTexture(GL_TEXTURE_2D, scene->blurSwapTex[!scene->horizontalBlur]);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        scene->horizontalBlur = !scene->horizontalBlur;
    }
}

void drawFullScreenQuad(Scene* scene) {
    glViewport(0, 0, scene->windowData.viewportWidth, scene->windowData.viewportHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, scene->editorFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(scene->postProcessShader);
    glUniform1f(uniform_location::kPExposure, scene->exposure);
    glUniform1f(uniform_location::kPBloomAmount, scene->bloomAmount);
    glUniform1f(uniform_location::kAOAmount, scene->AOAmount);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, scene->litColorTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, scene->blurSwapTex[scene->horizontalBlur]);
    glBindVertexArray(scene->fullscreenVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void createPickingFBO(Scene* scene) {
    unsigned int width = scene->windowData.width;
    unsigned int height = scene->windowData.height;

    glGenFramebuffers(1, &scene->pickingFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, scene->pickingFBO);

    glGenTextures(1, &scene->pickingTex);
    glBindTexture(GL_TEXTURE_2D, scene->pickingTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, scene->pickingTex, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenRenderbuffers(1, &scene->pickingRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, scene->pickingRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, scene->pickingRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void createShadowMapDepthBuffers(Scene* scene) {
    for (SpotLight& light : scene->spotLights) {
        glGenFramebuffers(1, &light.depthFrameBuffer);
        glGenTextures(1, &light.depthTex);
        glBindTexture(GL_TEXTURE_2D, light.depthTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, light.shadowWidth, light.shadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
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
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, light.shadowWidth, light.shadowHeight, 0, GL_RED, GL_FLOAT, NULL);
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

void createForwardBuffer(Scene* scene) {
    unsigned int width = scene->windowData.viewportWidth;
    unsigned int height = scene->windowData.viewportHeight;
    unsigned int attachments[4] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};

    glGenFramebuffers(1, &scene->litFBO);
    glGenTextures(1, &scene->litColorTex);
    glGenTextures(1, &scene->bloomSSAOTex);
    glGenTextures(1, &scene->ssaoNormalTex);
    glGenTextures(1, &scene->ssaoPosTex);
    glGenRenderbuffers(1, &scene->litRBO);

    glBindFramebuffer(GL_FRAMEBUFFER, scene->litFBO);

    glBindTexture(GL_TEXTURE_2D, scene->litColorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, scene->bloomSSAOTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, scene->ssaoPosTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // normal color buffer
    glBindTexture(GL_TEXTURE_2D, scene->ssaoNormalTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindRenderbuffer(GL_RENDERBUFFER, scene->litRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, scene->litColorTex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, scene->bloomSSAOTex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, scene->ssaoPosTex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, scene->ssaoNormalTex, 0);
    glDrawBuffers(4, attachments);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, scene->litRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER:: GBuffer framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void createEditorBuffer(Scene* scene) {
    GLint width = scene->windowData.viewportWidth;
    GLint height = scene->windowData.viewportHeight;

    glGenFramebuffers(1, &scene->editorFBO);
    glGenTextures(1, &scene->editorTex);
    glGenRenderbuffers(1, &scene->editorRBO);

    glBindFramebuffer(GL_FRAMEBUFFER, scene->editorFBO);
    glBindTexture(GL_TEXTURE_2D, scene->editorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, scene->editorTex, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, scene->editorRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, scene->editorRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER::Editor framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void createSSAOBuffer(Scene* scene) {
    glGenFramebuffers(1, &scene->ssaoFBO);
    glGenTextures(1, &scene->blurTex);

    glBindFramebuffer(GL_FRAMEBUFFER, scene->ssaoFBO);
    glBindTexture(GL_TEXTURE_2D, scene->blurTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, scene->windowData.viewportWidth, scene->windowData.viewportHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, scene->blurTex, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER:: SSAO Color framebuffer is not complete!" << std::endl;
    }
}

void createBlurBuffers(Scene* scene) {
    unsigned int width = scene->windowData.viewportWidth;
    unsigned int height = scene->windowData.viewportHeight;

    glGenFramebuffers(2, scene->blurFBO);
    glGenTextures(2, scene->blurSwapTex);

    for (unsigned int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, scene->blurFBO[i]);
        glBindTexture(GL_TEXTURE_2D, scene->blurSwapTex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, scene->blurSwapTex[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "ERROR::FRAMEBUFFER:: Blur framebuffer is not complete!" << std::endl;
        }
    }
}

void resizeBuffers(Scene* scene) {
    uint32_t width = scene->windowData.viewportWidth;
    uint32_t height = scene->windowData.viewportHeight;
    unsigned int attachments[4] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};

    glBindFramebuffer(GL_FRAMEBUFFER, scene->litFBO);
    glBindTexture(GL_TEXTURE_2D, scene->litColorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, scene->bloomSSAOTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, scene->ssaoPosTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, scene->ssaoNormalTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindRenderbuffer(GL_RENDERBUFFER, scene->litRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, scene->litColorTex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, scene->bloomSSAOTex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, scene->ssaoPosTex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, scene->ssaoNormalTex, 0);

    glDrawBuffers(4, attachments);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, scene->litRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER:: GBuffer framebuffer is not complete!" << std::endl;
    }

    for (unsigned int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, scene->blurFBO[i]);
        glBindTexture(GL_TEXTURE_2D, scene->blurSwapTex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, scene->blurSwapTex[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "ERROR::FRAMEBUFFER:: Blur framebuffer is not complete!" << std::endl;
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, scene->pickingFBO);
    glBindTexture(GL_TEXTURE_2D, scene->pickingTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, scene->pickingTex, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, scene->pickingRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, scene->pickingRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER:: Depth framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, scene->ssaoFBO);
    glBindTexture(GL_TEXTURE_2D, scene->blurTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, scene->windowData.viewportWidth, scene->windowData.viewportHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, scene->blurTex, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER:: SSAO Color framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, scene->editorFBO);
    glBindTexture(GL_TEXTURE_2D, scene->editorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, scene->editorTex, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, scene->editorRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, scene->editorRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER::Editor framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void createFullScreenQuad(Scene* scene) {
    float quadVertices[] = {
        -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,   // top-left
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,  // bottom-left
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f,    // top-right
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f,   // bottom-right
    };
    glGenVertexArrays(1, &scene->fullscreenVAO);
    glGenBuffers(1, &scene->fullscreenVBO);

    glBindVertexArray(scene->fullscreenVAO);
    glBindBuffer(GL_ARRAY_BUFFER, scene->fullscreenVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(vertex_attribute_location::kVertexPosition);
    glVertexAttribPointer(vertex_attribute_location::kVertexPosition, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(vertex_attribute_location::kVertexTexCoord);
    glVertexAttribPointer(vertex_attribute_location::kVertexTexCoord, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
}

float ourLerp(float a, float b, float f) {
    return a + f * (b - a);
}

void generateSSAOKernel(Scene* scene) {
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
    std::default_random_engine generator;

    scene->ssaoKernel.clear();
    scene->ssaoNoise.clear();

    for (unsigned int i = 0; i < 8; i++) {
        // Cosine-weighted hemisphere sampling
        float xi1 = randomFloats(generator);
        float xi2 = randomFloats(generator);

        float theta = acos(sqrt(1.0f - xi1));  // inverse CDF for cosine-weighted
        float phi = 2.0f * M_PI * xi2;

        glm::vec3 sample;
        sample.x = sin(theta) * cos(phi);
        sample.y = sin(theta) * sin(phi);
        sample.z = cos(theta);

        // Apply random scaling to bring samples closer to origin
        float scale = static_cast<float>(i) / 8.0f;
        // Stronger bias toward center → scale^3 instead of scale^2
        scale = ourLerp(0.05f, 1.0f, scale * scale * scale);
        sample *= scale;

        scene->ssaoKernel.push_back(sample);
    }

    // noise texture (same as before)
    for (unsigned int i = 0; i < 16; i++) {
        glm::vec3 noise(randomFloats(generator) * 2.0f - 1.0f,
                        randomFloats(generator) * 2.0f - 1.0f,
                        0.0f);  // rotate around z-axis (in tangent space)
        scene->ssaoNoise.push_back(noise);
    }

    glGenTextures(1, &scene->ssaoNoiseTex);
    glBindTexture(GL_TEXTURE_2D, scene->ssaoNoiseTex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGB, GL_FLOAT, &scene->ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glUseProgram(scene->ssaoShader);
    for (unsigned int i = 0; i < 32; i++) {
        glUniform3fv(glGetUniformLocation(scene->ssaoShader, ("samples[" + std::to_string(i) + "]").c_str()),
                     1, glm::value_ptr(scene->ssaoKernel[i]));
    }
}

void deleteBuffers(Scene* scene) {
    unsigned int textures[5] = {scene->pickingTex, scene->litColorTex, scene->bloomSSAOTex, scene->blurTex, scene->ssaoNoiseTex};
    unsigned int rbo[2] = {scene->pickingRBO, scene->litRBO};
    unsigned int frameBuffers[3] = {scene->pickingFBO, scene->ssaoFBO, scene->litFBO};

    glDeleteTextures(5, textures);
    glDeleteRenderbuffers(2, rbo);
    glDeleteFramebuffers(3, frameBuffers);
    glDeleteShader(scene->lightingShader);
    glDeleteShader(scene->pickingShader);
    glDeleteShader(scene->postProcessShader);
    glDeleteShader(scene->blurShader);
    glDeleteShader(scene->depthShader);
    glDeleteShader(scene->ssaoShader);

    glDeleteBuffers(1, &scene->fullscreenVBO);
    glDeleteVertexArrays(1, &scene->fullscreenVAO);
    for (Texture& tex : scene->textures) {
        glDeleteTextures(1, &tex.id);
    }

    for (auto pair : scene->meshMap) {
        Mesh* mesh = pair.second;
        glDeleteBuffers(1, &mesh->EBO);
        glDeleteBuffers(1, &mesh->VBO);
        glDeleteVertexArrays(1, &mesh->VAO);
    }
}