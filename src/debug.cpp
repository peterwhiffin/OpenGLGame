#include <sstream>
#include "debug.h"
#include "transform.h"
#include "player.h"
#include "sceneloader.h"

void checkPicker(Scene* scene, glm::dvec2 pickPosition) {
    if (!scene->isPicking) {
        return;
    }

    scene->isPicking = false;
    std::cout << "is picking" << std::endl;
    unsigned char pixel[3];
    glBindFramebuffer(GL_FRAMEBUFFER, scene->pickingFBO);
    glReadPixels(pickPosition.x, scene->windowData.height - pickPosition.y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
    uint32_t id = pixel[0] + (pixel[1] << 8) + (pixel[2] << 16);
    scene->nodeClicked = INVALID_ID;
    /*
        for (int i = 0; i < scene->entities.size(); i++) {
            if (scene->entities[i].entityID == id) {
                nodeClicked = id;
            }
        } */

    // std::cout << "ID: " << id << std::endl;

    if (scene->entityIndexMap.count(id)) {
        // std::cout << "id found" << std::endl;
        scene->nodeClicked = id;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void createImGuiEntityTree(Scene* scene, uint32_t entityID, ImGuiTreeNodeFlags node_flags) {
    Entity* entity = getEntity(scene, entityID);

    ImGui::PushID(entityID);
    std::stringstream ss;
    ss << entity->name << " - " << entity->entityID;
    std::string title = ss.str();
    bool node_open = ImGui::TreeNodeEx(title.c_str(), node_flags);

    if (ImGui::IsItemClicked()) {
        scene->nodeClicked = entity->entityID;
    }

    if (node_open) {
        Transform* transform = getTransform(scene, entityID);

        glm::vec3 position = transform->localPosition;
        ImGui::DragFloat3("Pos", glm::value_ptr(position), 0.1f, -1000.0f, 1000.0f);

        setLocalPosition(scene, entityID, position);

        PointLight* light = getPointLight(scene, entityID);
        if (light != nullptr) {
            ImGui::DragFloat("brightness", &light->brightness, 0.01f, 0.0f, 1000.0f);
        }

        BoxCollider* collider = getBoxCollider(scene, entityID);
        if (collider != nullptr) {
        }

        SpotLight* spotLight = getSpotLight(scene, entityID);
        if (spotLight != nullptr) {
            ImGui::DragFloat("inner cutoff", &spotLight->cutoff, 0.01f, 0.0f, spotLight->outerCutoff - 0.01f);
            ImGui::DragFloat("outer cutoff", &spotLight->outerCutoff, 0.01f, spotLight->cutoff + 0.01f, 180.0f);
            ImGui::DragFloat("brightness", &spotLight->brightness, 0.01f, 0.0f, 100.0f);
        }

        if (transform->parentEntityID != INVALID_ID) {
            entity = getEntity(scene, transform->parentEntityID);
            ImGui::Text("Parent: %s - %i", entity->name, entity->entityID);
        }

        if (ImGui::Button("Delete Entity", ImVec2(55, 35))) {
            destroyEntity(scene, entityID);
        }
        // ImGui::Text("MeshRenderer: %s", scene->renderers[it->second].mesh->name);

        for (uint32_t childEntityID : transform->childEntityIds) {
            createImGuiEntityTree(scene, childEntityID, node_flags);
        }

        ImGui::TreePop();
    }

    ImGui::PopID();
}

void buildImGui(Scene* scene, ImGuiTreeNodeFlags node_flags, Player* player) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("ImGui");

    if (scene->timeAccum >= 1.0f) {
        scene->FPS = scene->frameCount / scene->timeAccum;
        scene->frameTime = scene->timeAccum / scene->frameCount;
        scene->timeAccum = 0.0f;
        scene->frameCount = 0;
    } else {
        scene->timeAccum += scene->deltaTime;
        scene->frameCount++;
    }

    ImGui::Text("FPS: %.0f / FrameTime: %.6f", scene->FPS, scene->frameTime);
    ImGui::SliderFloat("Move Speed", &player->moveSpeed, 0.0f, 45.0f);
    ImGui::InputFloat("jump height", &player->jumpHeight);
    ImGui::InputFloat("gravity", &scene->gravity);
    ImGui::DragFloat("Normal Strength", &scene->normalStrength, 0.01f, 0, 100.0f);
    ImGui::DragFloat("Exposure", &scene->exposure, 0.01f, 0, 1000.0f);
    ImGui::DragFloat("Bloom Threshold", &scene->bloomThreshold, 0.01f, 0, 100.0f);
    ImGui::DragFloat("Bloom Amount", &scene->bloomAmount, 0.01f, 0, 100.0f);
    ImGui::DragFloat("Ambient", &scene->ambient, 0.01f, 0, 100.0f);

    if (ImGui::Button("Save Scene", ImVec2(75, 40))) {
        saveScene(scene);
    }
    // ImGui::Image((ImTextureID)(intptr_t)scene->gNormal, ImVec2(200, 200));
    for (int i = 0; i < scene->transforms.size(); i++) {
        if (scene->transforms[i].parentEntityID == INVALID_ID) {
            createImGuiEntityTree(scene, scene->transforms[i].entityID, node_flags);
        }
    }

    ImGui::End();
}

void drawDebug(Scene* scene, ImGuiTreeNodeFlags nodeFlags, Player* player) {
    buildImGui(scene, nodeFlags, player);
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}