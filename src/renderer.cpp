#include "transform.h"
#include "shader.h"
#include "renderer.h"

void drawPickingScene(Scene* scene, unsigned int pickingFBO, unsigned int pickingShader) {
    Camera* camera = scene->cameras[0];

    glBindFramebuffer(GL_FRAMEBUFFER, pickingFBO);
    glViewport(0, 0, scene->windowData.width, scene->windowData.height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (MeshRenderer renderer : scene->meshRenderers) {
        Transform* transform = getTransform(scene, renderer.entityID);

        glm::mat4 model = transform->worldTransform;
        glBindVertexArray(renderer.mesh->VAO);

        for (SubMesh* subMesh : renderer.mesh->subMeshes) {
            unsigned char r = renderer.entityID & 0xFF;
            unsigned char g = (renderer.entityID >> 8) & 0xFF;
            unsigned char b = (renderer.entityID >> 16) & 0xFF;
            glm::vec3 idColor = glm::vec3(r, g, b) / 255.0f;

            glUseProgram(pickingShader);
            glUniformMatrix4fv(uniform_location::kModelMatrix, 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix4fv(uniform_location::kViewMatrix, 1, GL_FALSE, glm::value_ptr(camera->viewMatrix));
            glUniformMatrix4fv(uniform_location::kProjectionMatrix, 1, GL_FALSE, glm::value_ptr(camera->projectionMatrix));
            glUniform3fv(uniform_location::kBaseColor, 1, glm::value_ptr(idColor));

            glDrawElements(GL_TRIANGLES, subMesh->indexCount, GL_UNSIGNED_INT, (void*)(subMesh->indexOffset * sizeof(unsigned int)));
        }

        glBindVertexArray(0);
    }
}

void drawScene(Scene* scene, uint32_t nodeClicked) {
    Camera* camera = scene->cameras[0];

    glBindFramebuffer(GL_FRAMEBUFFER, scene->forwardBuffer);
    glViewport(0, 0, scene->windowData.width, scene->windowData.height);
    glClearColor(0.34, 0.34, 0.8, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(scene->litForward);
    glUniform1i(glGetUniformLocation(scene->litForward, "dirLight.enabled"), scene->sun.isEnabled);
    glUniform3fv(glGetUniformLocation(scene->litForward, "dirLight.ambient"), 1, glm::value_ptr(scene->sun.ambient * scene->sun.ambientBrightness));
    glUniform3fv(glGetUniformLocation(scene->litForward, "dirLight.diffuse"), 1, glm::value_ptr(scene->sun.diffuse * scene->sun.diffuseBrightness));
    glUniform3fv(glGetUniformLocation(scene->litForward, "dirLight.specular"), 1, glm::value_ptr(scene->sun.specular * scene->sun.diffuseBrightness));

    for (int i = 0; i < scene->pointLights.size(); i++) {
        PointLight* pointLight = &scene->pointLights[i];
        std::string locationBase = "pointLights[" + std::to_string(i) + "]";
        glUniform3fv(glGetUniformLocation(scene->litForward, (locationBase + ".position").c_str()), 1, glm::value_ptr(getPosition(scene, pointLight->entityID)));
        glUniform3fv(glGetUniformLocation(scene->litForward, (locationBase + ".diffuse").c_str()), 1, glm::value_ptr(pointLight->diffuse * pointLight->brightness));
        glUniform3fv(glGetUniformLocation(scene->litForward, (locationBase + ".ambient").c_str()), 1, glm::value_ptr(pointLight->ambient * scene->ambient));
        glUniform3fv(glGetUniformLocation(scene->litForward, (locationBase + ".specular").c_str()), 1, glm::value_ptr(pointLight->specular * pointLight->brightness));
    }

    glUniform1f(uniform_location::kBloomThreshold, scene->bloomThreshold);

    glUniformMatrix4fv(uniform_location::kViewMatrix, 1, GL_FALSE, glm::value_ptr(camera->viewMatrix));
    glUniformMatrix4fv(uniform_location::kProjectionMatrix, 1, GL_FALSE, glm::value_ptr(camera->projectionMatrix));
    glUniform3fv(uniform_location::kViewPos, 1, glm::value_ptr(getLocalPosition(scene, camera->entityID)));
    glUniform1f(uniform_location::kShininess, 32.0f);

    glUniform1f(uniform_location::kNormalStrength, scene->normalStrength);

    for (int i = 0; i < scene->meshRenderers.size(); i++) {
        MeshRenderer* renderer = &scene->meshRenderers[i];
        glm::mat4 model = getTransform(scene, renderer->entityID)->worldTransform;
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
        glUniformMatrix4fv(uniform_location::kModelMatrix, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix3fv(uniform_location::kNormalMatrix, 1, GL_FALSE, glm::value_ptr(normalMatrix));

        for (SubMesh* subMesh : renderer->mesh->subMeshes) {
            unsigned int shader = subMesh->material.shader;
            glm::vec4 baseColor = (renderer->entityID == nodeClicked) ? glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) : subMesh->material.baseColor;

            glUseProgram(shader);
            glUniform4fv(uniform_location::kBaseColor, 1, glm::value_ptr(baseColor));
            glActiveTexture(GL_TEXTURE0 + uniform_location::kTextureDiffuseUnit);
            glBindTexture(GL_TEXTURE_2D, subMesh->material.textures[0].id);
            glActiveTexture(GL_TEXTURE0 + uniform_location::kTextureSpecularUnit);
            glBindTexture(GL_TEXTURE_2D, subMesh->material.textures[1].id);
            glActiveTexture(GL_TEXTURE0 + uniform_location::kTextureNormalUnit);
            glBindTexture(GL_TEXTURE_2D, subMesh->material.textures[2].id);

            glBindVertexArray(renderer->mesh->VAO);
            glDrawElements(GL_TRIANGLES, subMesh->indexCount, GL_UNSIGNED_INT, (void*)(subMesh->indexOffset * sizeof(unsigned int)));
        }

        glBindVertexArray(0);
    }
}

void drawFullScreenQuad(Scene* scene) {
    unsigned int width = scene->windowData.width;
    unsigned int height = scene->windowData.height;

    glViewport(0, 0, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(scene->postProcess);
    glUniform1f(uniform_location::kPExposure, scene->exposure);
    glUniform1f(uniform_location::kPBloomAmount, scene->bloomAmount);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, scene->forwardColor);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, scene->blurBuffer[scene->horizontalBlur]);
    glBindVertexArray(scene->fullscreenVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void createPickingFBO(Scene* scene, unsigned int* fbo, unsigned int* rbo, unsigned int* texture) {
    unsigned int width = scene->windowData.width;
    unsigned int height = scene->windowData.height;

    glGenFramebuffers(1, fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, *fbo);

    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *texture, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenRenderbuffers(1, rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, *rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, *rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void setFlags() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void createBlurBuffers(Scene* scene) {
    unsigned int width = scene->windowData.width;
    unsigned int height = scene->windowData.height;

    glGenFramebuffers(2, scene->blurFBO);
    glGenTextures(2, scene->blurBuffer);

    for (unsigned int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, scene->blurFBO[i]);
        glBindTexture(GL_TEXTURE_2D, scene->blurBuffer[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, scene->blurBuffer[i], 0);
    }
}
void createForwardBuffer(Scene* scene) {
    unsigned int width = scene->windowData.width;
    unsigned int height = scene->windowData.height;
    unsigned int attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};

    glGenFramebuffers(1, &scene->forwardBuffer);
    glGenTextures(1, &scene->forwardColor);
    glGenTextures(1, &scene->forwardBloom);
    glGenRenderbuffers(1, &scene->forwardDepth);

    glBindFramebuffer(GL_FRAMEBUFFER, scene->forwardBuffer);

    glBindTexture(GL_TEXTURE_2D, scene->forwardColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, scene->forwardBloom);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindRenderbuffer(GL_RENDERBUFFER, scene->forwardDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, scene->forwardColor, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, scene->forwardBloom, 0);

    glDrawBuffers(2, attachments);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, scene->forwardDepth);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::FRAMEBUFFER:: GBuffer framebuffer is not complete!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void drawBlurPass(Scene* scene) {
    scene->horizontalBlur = true;
    bool firstIteration = true;
    int amount = 10;
    glUseProgram(scene->blurPass);
    glActiveTexture(GL_TEXTURE0);

    for (unsigned int i = 0; i < amount; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, scene->blurFBO[scene->horizontalBlur]);
        glUniform1i(uniform_location::kBHorizontal, scene->horizontalBlur);
        glBindTexture(GL_TEXTURE_2D, firstIteration ? scene->forwardBloom : scene->blurBuffer[!scene->horizontalBlur]);
        glBindVertexArray(scene->fullscreenVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
        scene->horizontalBlur = !scene->horizontalBlur;
        if (firstIteration) {
            firstIteration = false;
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void createFullScreenQuad(Scene* scene) {
    float quadVertices[] = {
        -1.0f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        -1.0f,
        -1.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
        1.0f,
        0.0f,
        1.0f,
        1.0f,
        1.0f,
        -1.0f,
        0.0f,
        1.0f,
        0.0f,
    };

    glGenVertexArrays(1, &scene->fullscreenVAO);
    glGenBuffers(1, &scene->fullscreenVBO);

    glBindVertexArray(scene->fullscreenVAO);
    glBindBuffer(GL_ARRAY_BUFFER, scene->fullscreenVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(vertex_attribute_location::kPVertexPosition);
    glVertexAttribPointer(vertex_attribute_location::kPVertexPosition, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(vertex_attribute_location::kPVertexTexCoord);
    glVertexAttribPointer(vertex_attribute_location::kPVertexTexCoord, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
}