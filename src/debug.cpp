#include "debug.h"
#include "transform.h"

bool searchEntities(Entity* entity, unsigned int id, Entity* nodeClicked) {
    if (entity->id == id) {
        nodeClicked = entity;
        return true;
    }

    for (Transform* childEntity : entity->transform.children) {
        if (searchEntities(childEntity->entity, id, nodeClicked)) {
            return true;
        }
    }

    nodeClicked = nullptr;
    return false;
}

void checkPicker(glm::dvec2 pickPosition, WindowData* windowData, std::vector<Entity>& entities, Entity* nodeClicked) {
    unsigned char pixel[3];
    glReadPixels(pickPosition.x, windowData->height - pickPosition.y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
    unsigned int id = pixel[0] + (pixel[1] << 8) + (pixel[2] << 16);

    for (Entity entity : entities) {
        if (searchEntities(&entity, id, nodeClicked)) {
            break;
        }
    }
}

void createImGuiEntityTree(Entity* entity, ImGuiTreeNodeFlags node_flags, Entity* node_clicked) {
    ImGui::PushID(entity);
    bool node_open = ImGui::TreeNodeEx(entity->name.c_str(), node_flags);

    if (ImGui::IsItemClicked()) {
        node_clicked = entity;
    }

    if (node_open) {
        ImGui::Text("X: (%.1f), Y: (%.1f), Z: (%.1f)", getPosition(&entity->transform).x, getPosition(&entity->transform).y, getPosition(&entity->transform).z);

        for (Transform* child : entity->transform.children) {
            createImGuiEntityTree(child->entity, node_flags, node_clicked);
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void buildImGui(std::vector<Entity>& entities, ImGuiTreeNodeFlags node_flags, Entity* nodeClicked, Player* player, DirectionalLight* sun, float* gravity, bool* enableDirLight) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("ImGui");
    ImGui::SliderFloat("Move Speed", &player->moveSpeed, 0.0f, 45.0f);
    ImGui::InputFloat("jump height", &player->jumpHeight);
    ImGui::InputFloat("gravity", gravity);
    ImGui::Checkbox("Enable Directional Light", enableDirLight);
    if (enableDirLight) {
        ImGui::SliderFloat("Directional Light Brightness", &sun->diffuseBrightness, 0.0f, 10.0f);
        ImGui::SliderFloat("Ambient Brightness", &sun->ambientBrightness, 0.0f, 3.0f);
    }
    // ImGui::Image((ImTextureID)(intptr_t)pickingTexture, ImVec2(200, 200));
    for (Entity entity : entities) {
        createImGuiEntityTree(&entity, node_flags, nodeClicked);
    }

    ImGui::End();
}