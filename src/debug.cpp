#include "debug.h"
#include "transform.h"

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

void createImGuiEntityTree(Scene* scene, Entity* entity, ImGuiTreeNodeFlags node_flags, uint32_t node_clicked) {
    ImGui::PushID(entity);
    bool node_open = ImGui::TreeNodeEx(entity->name.c_str(), node_flags);

    if (ImGui::IsItemClicked()) {
        node_clicked = entity->id;
    }

    if (node_open) {
        size_t index = scene->transformIndices[entity->id];
        Transform* transform = &scene->transforms[index];
        ImGui::Text("X: (%.1f), Y: (%.1f), Z: (%.1f)", getPosition(scene, entity->id).x, getPosition(scene, entity->id).y, getPosition(scene, entity->id).z);

        for (uint32_t childEntityID : transform->childEntityIds) {
            size_t index = scene->entityIndices[childEntityID];
            Entity* entity = &scene->entities[index];
            createImGuiEntityTree(scene, entity, node_flags, node_clicked);
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
    ImGui::SliderFloat("Move Speed", &player->moveSpeed, 0.0f, 45.0f);
    ImGui::InputFloat("jump height", &player->jumpHeight);
    ImGui::InputFloat("gravity", &scene->gravity);
    ImGui::Checkbox("Enable Directional Light", &scene->sun.isEnabled);
    if (scene->sun.isEnabled) {
        ImGui::SliderFloat("Directional Light Brightness", &scene->sun.diffuseBrightness, 0.0f, 10.0f);
        ImGui::SliderFloat("Ambient Brightness", &scene->sun.ambientBrightness, 0.0f, 3.0f);
    }
    // ImGui::Image((ImTextureID)(intptr_t)pickingTexture, ImVec2(200, 200));
    for (int i = 0; i < scene->entities.size(); i++) {
        createImGuiEntityTree(scene, &scene->entities[i], node_flags, nodeClicked);
    }

    ImGui::End();
}