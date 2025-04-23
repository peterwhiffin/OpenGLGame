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
        Transform* transform = nullptr;
        getTransform(scene, renderer.entityID, &transform);

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

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, scene->windowData.width, scene->windowData.height);
    glClearColor(0.34, 0.34, 0.8, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (int i = 0; i < scene->meshRenderers.size(); i++) {
        MeshRenderer* renderer = &scene->meshRenderers[i];
        Transform* transform = nullptr;
        getTransform(scene, renderer->entityID, &transform);
        if (renderer->mesh->name == "Trashcan Base") {
            // std::cout << "we breakin" << std::endl;
        }
        glm::mat4 model = transform->worldTransform;
        glm::mat4 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));

        glBindVertexArray(renderer->mesh->VAO);

        for (SubMesh* subMesh : renderer->mesh->subMeshes) {
            unsigned int shader = subMesh->material.shader;
            glm::vec4 baseColor = (renderer->entityID == nodeClicked) ? glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) : subMesh->material.baseColor;

            glUseProgram(shader);
            glUniformMatrix4fv(uniform_location::kModelMatrix, 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix4fv(uniform_location::kViewMatrix, 1, GL_FALSE, glm::value_ptr(camera->viewMatrix));
            glUniformMatrix4fv(uniform_location::kProjectionMatrix, 1, GL_FALSE, glm::value_ptr(camera->projectionMatrix));
            glUniformMatrix4fv(uniform_location::kNormalMatrix, 1, GL_FALSE, glm::value_ptr(normalMatrix));
            glUniform4fv(uniform_location::kBaseColor, 1, glm::value_ptr(baseColor));
            // glUniform1f(uniform_location::kShininess, subMesh->material.shininess);
            glUniform1f(uniform_location::kShininess, 512.0f);
            glUniform3fv(uniform_location::kViewPos, 1, glm::value_ptr(getLocalPosition(scene, camera->entityID)));

            glUniform1i(glGetUniformLocation(shader, "dirLight.enabled"), scene->sun.isEnabled);
            glUniform3fv(glGetUniformLocation(shader, "dirLight.ambient"), 1, glm::value_ptr(scene->sun.ambient * scene->sun.ambientBrightness));
            glUniform3fv(glGetUniformLocation(shader, "dirLight.diffuse"), 1, glm::value_ptr(scene->sun.diffuse * scene->sun.diffuseBrightness));
            glUniform3fv(glGetUniformLocation(shader, "dirLight.specular"), 1, glm::value_ptr(scene->sun.specular * scene->sun.diffuseBrightness));
            glActiveTexture(GL_TEXTURE0 + uniform_location::kTextureDiffuseUnit);
            glBindTexture(GL_TEXTURE_2D, subMesh->material.textures[0].id);
            glActiveTexture(GL_TEXTURE0 + uniform_location::kTextureSpecularUnit);
            glBindTexture(GL_TEXTURE_2D, subMesh->material.textures[1].id);
            glDrawElements(GL_TRIANGLES, subMesh->indexCount, GL_UNSIGNED_INT, (void*)(subMesh->indexOffset * sizeof(unsigned int)));
        }

        glBindVertexArray(0);
    }
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