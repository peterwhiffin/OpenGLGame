#include <random>
#include "transform.h"
#include "shader.h"
#include "renderer.h"

void setFlags() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void drawPickingScene(Scene* scene) {
    Camera* camera = scene->cameras[0];

    glBindFramebuffer(GL_FRAMEBUFFER, scene->pickingFBO);
    glViewport(0, 0, scene->windowData.viewportWidth, scene->windowData.viewportHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (MeshRenderer& renderer : scene->meshRenderers) {
        Transform* transform = getTransform(scene, renderer.entityID);

        glm::mat4 model = transform->worldTransform;
        glBindVertexArray(renderer.mesh->VAO);

        for (SubMesh& subMesh : renderer.mesh->subMeshes) {
            unsigned char r = renderer.entityID & 0xFF;
            unsigned char g = (renderer.entityID >> 8) & 0xFF;
            unsigned char b = (renderer.entityID >> 16) & 0xFF;
            glm::vec3 idColor = glm::vec3(r, g, b) / 255.0f;

            glUseProgram(scene->pickingShader);
            glUniformMatrix4fv(uniform_location::kModelMatrix, 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix4fv(uniform_location::kViewMatrix, 1, GL_FALSE, glm::value_ptr(camera->viewMatrix));
            glUniformMatrix4fv(uniform_location::kProjectionMatrix, 1, GL_FALSE, glm::value_ptr(camera->projectionMatrix));
            glUniform3fv(uniform_location::kColor, 1, glm::value_ptr(idColor));

            glDrawElements(GL_TRIANGLES, subMesh.indexCount, GL_UNSIGNED_INT, (void*)(subMesh.indexOffset * sizeof(unsigned int)));
        }

        glBindVertexArray(0);
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
        light.lightSpaceMatrix = projectionMatrix * viewMatrix;

        glUniformMatrix4fv(uniform_location::kViewMatrix, 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(uniform_location::kProjectionMatrix, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

        for (MeshRenderer& renderer : scene->meshRenderers) {
            glm::mat4 model = getTransform(scene, renderer.entityID)->worldTransform;
            glUniformMatrix4fv(uniform_location::kModelMatrix, 1, GL_FALSE, glm::value_ptr(model));
            glBindVertexArray(renderer.mesh->VAO);

            for (SubMesh& subMesh : renderer.mesh->subMeshes) {
                glDrawElements(GL_TRIANGLES, subMesh.indexCount, GL_UNSIGNED_INT, (void*)(subMesh.indexOffset * sizeof(unsigned int)));
            }
        }
    }
}

void drawScene(Scene* scene) {
    Camera* camera = scene->cameras[0];

    glBindFramebuffer(GL_FRAMEBUFFER, scene->litFBO);
    glViewport(0, 0, scene->windowData.viewportWidth, scene->windowData.viewportHeight);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(scene->lightingShader);

    for (int i = 0; i < scene->pointLights.size(); i++) {
        PointLight* pointLight = &scene->pointLights[i];
        std::string locationBase = "pointLights[" + std::to_string(i) + "]";
        glUniform3fv(glGetUniformLocation(scene->lightingShader, (locationBase + ".position").c_str()), 1, glm::value_ptr(getPosition(scene, pointLight->entityID)));
        glUniform3fv(glGetUniformLocation(scene->lightingShader, (locationBase + ".color").c_str()), 1, glm::value_ptr(pointLight->color * pointLight->brightness));
    }

    for (int i = 0; i < scene->spotLights.size(); i++) {
        SpotLight* spotLight = &scene->spotLights[i];
        std::string locationBase = "spotLights[" + std::to_string(i) + "]";
        glUniform3fv(glGetUniformLocation(scene->lightingShader, (locationBase + ".position").c_str()), 1, glm::value_ptr(getPosition(scene, spotLight->entityID)));
        glUniform3fv(glGetUniformLocation(scene->lightingShader, (locationBase + ".direction").c_str()), 1, glm::value_ptr(forward(scene, spotLight->entityID)));
        glUniform3fv(glGetUniformLocation(scene->lightingShader, (locationBase + ".color").c_str()), 1, glm::value_ptr(spotLight->color));
        glUniform1f(glGetUniformLocation(scene->lightingShader, (locationBase + ".brightness").c_str()), spotLight->brightness);
        glUniform1f(glGetUniformLocation(scene->lightingShader, (locationBase + ".cutOff").c_str()), glm::cos(glm::radians(spotLight->cutoff)));
        glUniform1f(glGetUniformLocation(scene->lightingShader, (locationBase + ".outerCutOff").c_str()), glm::cos(glm::radians(spotLight->outerCutoff)));
        glUniformMatrix4fv(glGetUniformLocation(scene->lightingShader, ("lightSpaceMatrix[" + std::to_string(i) + "]").c_str()), 1, GL_FALSE, glm::value_ptr(spotLight->lightSpaceMatrix));
        glActiveTexture(GL_TEXTURE0 + uniform_location::kTextureShadowMapUnit + i);
        glBindTexture(GL_TEXTURE_2D, spotLight->depthTex);
    }

    glUniform1f(uniform_location::kBloomThreshold, scene->bloomThreshold);

    glUniformMatrix4fv(uniform_location::kViewMatrix, 1, GL_FALSE, glm::value_ptr(camera->viewMatrix));
    glUniformMatrix4fv(uniform_location::kProjectionMatrix, 1, GL_FALSE, glm::value_ptr(camera->projectionMatrix));
    glUniform3fv(uniform_location::kViewPos, 1, glm::value_ptr(getLocalPosition(scene, camera->entityID)));

    glUniform1f(uniform_location::kNormalStrength, scene->normalStrength);
    glUniform1f(uniform_location::kMetallicStrength, 1.0f);
    glUniform1f(uniform_location::kRoughnessStrength, 1.0f);
    glUniform1f(uniform_location::kAOStrength, 1.0f);

    for (MeshRenderer& renderer : scene->meshRenderers) {
        glm::mat4 model = getTransform(scene, renderer.entityID)->worldTransform;
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
        glm::vec3 baseColor = (renderer.entityID == scene->nodeClicked) ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 1.0f, 1.0f);
        glUniform3fv(uniform_location::kColor, 1, glm::value_ptr(baseColor));
        glUniformMatrix4fv(uniform_location::kModelMatrix, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix3fv(uniform_location::kNormalMatrix, 1, GL_FALSE, glm::value_ptr(normalMatrix));
        glBindVertexArray(renderer.mesh->VAO);

        for (SubMesh& subMesh : renderer.mesh->subMeshes) {
            unsigned int shader = subMesh.material.shader;

            glActiveTexture(GL_TEXTURE0 + uniform_location::kTextureAlbedoUnit);
            glBindTexture(GL_TEXTURE_2D, subMesh.material.textures[0].id);
            glActiveTexture(GL_TEXTURE0 + uniform_location::kTextureRoughnessUnit);
            glBindTexture(GL_TEXTURE_2D, subMesh.material.textures[1].id);
            glActiveTexture(GL_TEXTURE0 + uniform_location::kTextureMetallicUnit);
            glBindTexture(GL_TEXTURE_2D, subMesh.material.textures[2].id);
            glActiveTexture(GL_TEXTURE0 + uniform_location::kTextureAOUnit);
            glBindTexture(GL_TEXTURE_2D, subMesh.material.textures[3].id);
            glActiveTexture(GL_TEXTURE0 + uniform_location::kTextureNormalUnit);
            glBindTexture(GL_TEXTURE_2D, subMesh.material.textures[4].id);
            glDrawElements(GL_TRIANGLES, subMesh.indexCount, GL_UNSIGNED_INT, (void*)(subMesh.indexOffset * sizeof(unsigned int)));
        }
    }
}

void drawSSAO(Scene* scene) {
    glViewport(0, 0, scene->windowData.viewportWidth, scene->windowData.viewportHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, scene->ssaoFBO);
    glClearColor(0.0, 0.0, 0.0, 0.0);
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

    glUniformMatrix4fv(5, 1, GL_FALSE, glm::value_ptr(scene->cameras[0]->projectionMatrix));
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

    glViewport(0, 0, scene->windowData.viewportWidth, scene->windowData.viewportHeight);
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

    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void drawFullScreenQuad(Scene* scene) {
    glViewport(0, 0, scene->windowData.width, scene->windowData.height);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_DEPTH_BUFFER_BIT);
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

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void createForwardBuffer(Scene* scene) {
    unsigned int width = scene->windowData.width;
    unsigned int height = scene->windowData.height;
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, scene->windowData.viewportWidth, scene->windowData.viewportHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // normal color buffer
    glBindTexture(GL_TEXTURE_2D, scene->ssaoNormalTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, scene->windowData.viewportWidth, scene->windowData.viewportHeight, 0, GL_RGBA, GL_FLOAT, NULL);
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
    unsigned int width = scene->windowData.width;
    unsigned int height = scene->windowData.height;

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

void generateSSAOKernel(Scene* scene) {
    std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
    std::default_random_engine generator;

    for (unsigned int i = 0; i < 64; i++) {
        glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        float scale = (float)i / 64.0f;
        scale = 0.1f + (scale * scale) * (1.0f - 0.1f);
        sample *= scale;
        scene->ssaoKernel.push_back(sample);
    }

    for (unsigned int i = 0; i < 16; i++) {
        glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f);  // rotate around z-axis (in tangent space)
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
    for (unsigned int i = 0; i < 64; i++) {
        glUniform3fv(glGetUniformLocation(scene->ssaoShader, ("samples[" + std::to_string(i) + "]").c_str()), 1, glm::value_ptr(scene->ssaoKernel[i]));
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