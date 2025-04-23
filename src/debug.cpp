#include <sstream>
#include "debug.h"
#include "transform.h"
#include "player.h"

void checkPicker(Scene* scene, glm::dvec2 pickPosition, uint32_t nodeClicked) {
    unsigned char pixel[3];
    glReadPixels(pickPosition.x, scene->windowData.height - pickPosition.y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
    uint32_t id = pixel[0] + (pixel[1] << 8) + (pixel[2] << 16);
    nodeClicked = INVALID_ID;

    for (int i = 0; i < scene->entities.size(); i++) {
        if (scene->entities[i].id == id) {
            nodeClicked = id;
        }
    }
}

void createImGuiEntityTree(Scene* scene, uint32_t entityID, ImGuiTreeNodeFlags node_flags, uint32_t node_clicked) {
    Entity* entity = getEntity(scene, entityID);

    ImGui::PushID(entityID);
    std::stringstream ss;
    ss << entity->name << " - " << entity->id;
    std::string title = ss.str();
    bool node_open = ImGui::TreeNodeEx(title.c_str(), node_flags);

    if (ImGui::IsItemClicked()) {
        node_clicked = entity->id;
    }

    if (node_open) {
        Transform* transform = getTransform(scene, entityID);

        glm::vec3 position = transform->localPosition;
        ImGui::SliderFloat3("Pos", glm::value_ptr(position), -100.0f, 100.0f);
        setLocalPosition(scene, entityID, position);

        PointLight* light = getPointLight(scene, entityID);
        if (light != nullptr) {
            ImGui::SliderFloat("brightness", &light->brightness, 0.0f, 10.0f);
        }

        if (transform->parentEntityID != INVALID_ID) {
            entity = getEntity(scene, transform->parentEntityID);
            ImGui::Text("Parent: %s - %i", entity->name, entity->id);
        }
        // ImGui::Text("MeshRenderer: %s", scene->renderers[it->second].mesh->name);

        for (uint32_t childEntityID : transform->childEntityIds) {
            createImGuiEntityTree(scene, childEntityID, node_flags, node_clicked);
        }

        ImGui::TreePop();
    }

    ImGui::PopID();
}

void buildImGui(Scene* scene, ImGuiTreeNodeFlags node_flags, uint32_t nodeClicked, Player* player) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("ImGui");

    if (scene->timeAccum >= 1.0f) {
        scene->FPS = scene->frameCount / scene->timeAccum;
        scene->timeAccum = 0.0f;
        scene->frameCount = 0;
    } else {
        scene->timeAccum += scene->deltaTime;
        scene->frameCount++;
    }

    ImGui::Text("FPS: %.0f", scene->FPS);
    ImGui::SliderFloat("Move Speed", &player->moveSpeed, 0.0f, 45.0f);
    ImGui::InputFloat("jump height", &player->jumpHeight);
    ImGui::InputFloat("gravity", &scene->gravity);
    ImGui::Checkbox("Enable Directional Light", &scene->sun.isEnabled);
    if (scene->sun.isEnabled) {
        ImGui::SliderFloat("Directional Light Brightness", &scene->sun.diffuseBrightness, 0.0f, 10.0f);
        ImGui::SliderFloat("Ambient Brightness", &scene->sun.ambientBrightness, 0.0f, 3.0f);
    }
    // ImGui::Image((ImTextureID)(intptr_t)pickingTexture, ImVec2(200, 200));
    for (int i = 0; i < scene->transforms.size(); i++) {
        if (scene->transforms[i].parentEntityID == INVALID_ID) {
            createImGuiEntityTree(scene, scene->transforms[i].entityID, node_flags, nodeClicked);
        }
    }

    ImGui::End();
}

void drawDebug(Scene* scene, ImGuiTreeNodeFlags nodeFlags, uint32_t nodeClicked, Player* player) {
    buildImGui(scene, nodeFlags, nodeClicked, player);
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}