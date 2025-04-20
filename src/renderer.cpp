#include "transform.h"
#include "shader.h"

void drawPickingScene(std::vector<MeshRenderer*>& renderers, Camera& camera, unsigned int pickingShader) {
    for (MeshRenderer* renderer : renderers) {
        glm::mat4 model = renderer->transform->worldTransform;
        glBindVertexArray(renderer->mesh->VAO);

        for (SubMesh* subMesh : renderer->mesh->subMeshes) {
            unsigned char r = renderer->entity->id & 0xFF;
            unsigned char g = (renderer->entity->id >> 8) & 0xFF;
            unsigned char b = (renderer->entity->id >> 16) & 0xFF;
            glm::vec3 idColor = glm::vec3(r, g, b) / 255.0f;

            glUseProgram(pickingShader);
            glUniformMatrix4fv(uniform_location::kModelMatrix, 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix4fv(uniform_location::kViewMatrix, 1, GL_FALSE, glm::value_ptr(camera.viewMatrix));
            glUniformMatrix4fv(uniform_location::kProjectionMatrix, 1, GL_FALSE, glm::value_ptr(camera.projectionMatrix));
            glUniform3fv(uniform_location::kBaseColor, 1, glm::value_ptr(idColor));

            glDrawElements(GL_TRIANGLES, subMesh->indexCount, GL_UNSIGNED_INT, (void*)(subMesh->indexOffset * sizeof(unsigned int)));
        }

        glBindVertexArray(0);
    }
}

void drawScene(std::vector<MeshRenderer*>& renderers, Camera& camera, Entity* nodeClicked, bool enableDirLight, DirectionalLight* sun) {
    for (MeshRenderer* renderer : renderers) {
        glm::mat4 model = renderer->transform->worldTransform;
        glm::mat4 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));

        glBindVertexArray(renderer->mesh->VAO);

        for (SubMesh* subMesh : renderer->mesh->subMeshes) {
            unsigned int shader = subMesh->material.shader;
            glm::vec4 baseColor = (renderer->entity == nodeClicked) ? glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) : subMesh->material.baseColor;

            glUseProgram(shader);
            glUniformMatrix4fv(uniform_location::kModelMatrix, 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix4fv(uniform_location::kViewMatrix, 1, GL_FALSE, glm::value_ptr(camera.viewMatrix));
            glUniformMatrix4fv(uniform_location::kProjectionMatrix, 1, GL_FALSE, glm::value_ptr(camera.projectionMatrix));
            glUniformMatrix4fv(uniform_location::kNormalMatrix, 1, GL_FALSE, glm::value_ptr(normalMatrix));
            glUniform4fv(uniform_location::kBaseColor, 1, glm::value_ptr(baseColor));
            // glUniform1f(uniform_location::kShininess, subMesh->material.shininess);
            glUniform1f(uniform_location::kShininess, 512.0f);
            glUniform3fv(uniform_location::kViewPos, 1, glm::value_ptr(getLocalPosition(camera.transform)));

            glUniform1i(glGetUniformLocation(shader, "dirLight.enabled"), enableDirLight);
            glUniform3fv(glGetUniformLocation(shader, "dirLight.ambient"), 1, glm::value_ptr(sun->ambient));
            glUniform3fv(glGetUniformLocation(shader, "dirLight.diffuse"), 1, glm::value_ptr(sun->diffuse * sun->brightness));
            glUniform3fv(glGetUniformLocation(shader, "dirLight.specular"), 1, glm::value_ptr(sun->specular * sun->brightness));
            glActiveTexture(GL_TEXTURE0 + uniform_location::kTextureDiffuseUnit);
            glBindTexture(GL_TEXTURE_2D, subMesh->material.textures[0].id);
            glActiveTexture(GL_TEXTURE0 + uniform_location::kTextureSpecularUnit);
            glBindTexture(GL_TEXTURE_2D, subMesh->material.textures[1].id);
            glDrawElements(GL_TRIANGLES, subMesh->indexCount, GL_UNSIGNED_INT, (void*)(subMesh->indexOffset * sizeof(unsigned int)));
        }

        glBindVertexArray(0);
    }
}
