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
    unsigned char pixel[3];
    glBindFramebuffer(GL_FRAMEBUFFER, scene->pickingFBO);
    glReadPixels(pickPosition.x, scene->windowData.height - pickPosition.y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
    uint32_t id = pixel[0] + (pixel[1] << 8) + (pixel[2] << 16);
    // scene->nodeClicked = INVALID_ID;
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
    Transform* transform = getTransform(scene, entityID);

    ImGui::PushID(entityID);

    node_flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;

    if (transform->childEntityIds.size() == 0) {
        node_flags |= ImGuiTreeNodeFlags_Leaf;
    }
    if (scene->nodeClicked == entityID) {
        node_flags |= ImGuiTreeNodeFlags_Selected;
    }

    std::string title = entity->name;
    bool node_open = ImGui::TreeNodeEx(title.c_str(), node_flags);
    if (ImGui::IsItemClicked()) {
        scene->nodeClicked = entity->entityID;
    }
    if (node_open) {
        for (uint32_t childEntityID : transform->childEntityIds) {
            createImGuiEntityTree(scene, childEntityID, node_flags);
        }

        ImGui::TreePop();
    }

    ImGui::PopID();
}

static void ShowExampleMenuFile(Scene* scene) {
    ImGui::MenuItem("(demo menu)", NULL, false, false);
    if (ImGui::MenuItem("New")) {
    }
    if (ImGui::MenuItem("Open", "Ctrl+O")) {
    }
    if (ImGui::BeginMenu("Open Recent")) {
        ImGui::MenuItem("fish_hat.c");
        ImGui::MenuItem("fish_hat.inl");
        ImGui::MenuItem("fish_hat.h");
        if (ImGui::BeginMenu("More..")) {
            ImGui::MenuItem("Hello");
            ImGui::MenuItem("Sailor");
            if (ImGui::BeginMenu("Recurse..")) {
                ShowExampleMenuFile(scene);
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }
    if (ImGui::MenuItem("Save", "Ctrl+S")) {
        saveScene(scene);
    }
    if (ImGui::MenuItem("Save As..")) {
    }

    ImGui::Separator();
    if (ImGui::BeginMenu("Options")) {
        static bool enabled = true;
        ImGui::MenuItem("Enabled", "", &enabled);
        ImGui::BeginChild("child", ImVec2(0, 60), ImGuiChildFlags_Borders);
        for (int i = 0; i < 10; i++)
            ImGui::Text("Scrolling Text %d", i);
        ImGui::EndChild();
        static float f = 0.5f;
        static int n = 0;
        ImGui::SliderFloat("Value", &f, 0.0f, 1.0f);
        ImGui::InputFloat("Input", &f, 0.1f);
        ImGui::Combo("Combo", &n, "Yes\0No\0Maybe\0\0");
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Colors")) {
        float sz = ImGui::GetTextLineHeight();
        for (int i = 0; i < ImGuiCol_COUNT; i++) {
            const char* name = ImGui::GetStyleColorName((ImGuiCol)i);
            ImVec2 p = ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + sz, p.y + sz), ImGui::GetColorU32((ImGuiCol)i));
            ImGui::Dummy(ImVec2(sz, sz));
            ImGui::SameLine();
            ImGui::MenuItem(name);
        }
        ImGui::EndMenu();
    }

    // Here we demonstrate appending again to the "Options" menu (which we already created above)
    // Of course in this demo it is a little bit silly that this function calls BeginMenu("Options") twice.
    // In a real code-base using it would make senses to use this feature from very different code locations.
    if (ImGui::BeginMenu("Options"))  // <-- Append!
    {
        static bool b = true;
        ImGui::Checkbox("SomeOption", &b);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Disabled", false))  // Disabled
    {
        IM_ASSERT(0);
    }
    if (ImGui::MenuItem("Checked", NULL, true)) {
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Quit", "Alt+F4")) {
    }
}

void buildImGui(Scene* scene, ImGuiTreeNodeFlags node_flags, Player* player) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

    if (scene->timeAccum >= 1.0f) {
        scene->FPS = scene->frameCount / scene->timeAccum;
        scene->frameTime = scene->timeAccum / scene->frameCount;
        scene->timeAccum = 0.0f;
        scene->frameCount = 0;
    } else {
        scene->timeAccum += scene->deltaTime;
        scene->frameCount++;
    }
    bool showDemo = true;
    ImGui::ShowDemoWindow(&showDemo);
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ShowExampleMenuFile(scene);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "CTRL+Z")) {
            }
            if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {
            }  // Disabled item
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "CTRL+X")) {
            }
            if (ImGui::MenuItem("Copy", "CTRL+C")) {
            }
            if (ImGui::MenuItem("Paste", "CTRL+V")) {
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("EditViewport", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse);
    ImGui::Text(("FPS: " + std::to_string(scene->FPS) + " / Frame Time: " + std::to_string(scene->frameTime)).c_str());
    // ImGui::Combo("Mode", &mode, modes, IM_ARRAYSIZE(modes));
    // ImGui::Button("Options");
    ImGui::Separator();
    ImVec2 availableSize = ImGui::GetContentRegionAvail();
    float aspectRatio = (float)scene->windowData.viewportWidth / scene->windowData.viewportHeight;

    ImVec2 imageSize = ImVec2(scene->windowData.viewportWidth, scene->windowData.viewportHeight);
    imageSize.y = glm::clamp(imageSize.y, 0.0f, availableSize.y);
    imageSize.x = imageSize.y * aspectRatio;

    if (imageSize.x > availableSize.x) {
        imageSize.x = availableSize.x;
        imageSize.y = imageSize.x * (1 / aspectRatio);
    }

    ImGui::SetCursorPos(ImVec2((availableSize.x - imageSize.x) / 2, (availableSize.y - imageSize.y) / 2));
    ImGui::Image((ImTextureID)(intptr_t)scene->editorTex, imageSize, ImVec2(0, 1), ImVec2(1, 0));
    ImGui::End();

    flags = ImGuiConfigFlags_DockingEnable;

    ImGui::PopStyleVar();
    ImGui::Begin("SceneHierarchy");
    for (int i = 0; i < scene->transforms.size(); i++) {
        if (scene->transforms[i].parentEntityID == INVALID_ID) {
            createImGuiEntityTree(scene, scene->transforms[i].entityID, node_flags);
        }
    }
    ImGui::End();

    ImGui::Begin("Inspector");
    unsigned int entityID = scene->nodeClicked;

    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable("Transform Table", 2, ImGuiTableFlags_SizingFixedSame)) {
            Transform* transform = getTransform(scene, entityID);
            vec3 position = transform->localPosition;
            vec3 worldPosition = transform->worldTransform.GetTranslation();
            vec3 rotation = transform->localRotation.GetEulerAngles();
            vec3 degrees = vec3(JPH::RadiansToDegrees(rotation.GetX()), JPH::RadiansToDegrees(rotation.GetX()), JPH::RadiansToDegrees(rotation.GetX()));
            vec3 scale = transform->localScale;
            ImGui::TableSetupColumn("##Label", ImGuiTableColumnFlags_None, 0.0f, 200.0f);
            ImGui::TableSetupColumn("##Widget", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("World Position");
            ImGui::TableSetColumnIndex(1);
            ImGui::DragFloat3("##worldposition", worldPosition.mF32, 0.1f);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Position");
            ImGui::TableSetColumnIndex(1);
            ImGui::DragFloat3("##Position", position.mF32, 0.1f);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Rotation");
            ImGui::TableSetColumnIndex(1);
            ImGui::DragFloat3("##Rotation", degrees.mF32, 0.1f);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Scale");
            ImGui::TableSetColumnIndex(1);
            ImGui::DragFloat3("##Scale", scale.mF32, 0.1f);

            vec3 radians = vec3(JPH::DegreesToRadians(degrees.GetX()), JPH::DegreesToRadians(degrees.GetY()), JPH::DegreesToRadians(degrees.GetZ()));
            setLocalPosition(scene, entityID, position);
            setLocalRotation(scene, entityID, quat::sEulerAngles(radians));
            setLocalScale(scene, entityID, scale);
            ImGui::EndTable();
        }
    }

    Animator* animator = getAnimator(scene, entityID);
    if (animator != nullptr) {
        if (ImGui::CollapsingHeader("Animator", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::BeginTable("Animator Table", 2, ImGuiTableFlags_SizingFixedSame)) {
                ImGui::TableSetupColumn("##Label", ImGuiTableColumnFlags_None, 0.0f, 200.0f);
                ImGui::TableSetupColumn("##Widget", ImGuiTableColumnFlags_WidthStretch);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Current Animation:");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text(animator->currentAnimation->name.c_str());

                if (ImGui::Button("Next Animation", ImVec2(30, 40))) {
                    animator->currentIndex++;
                    if (animator->currentIndex >= animator->animations.size()) {
                        animator->currentIndex = 0;
                    }

                    animator->currentAnimation = animator->animations[animator->currentIndex];
                    animator->playbackTime = 0.0f;
                }

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Playback Time:");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text(std::to_string(animator->playbackTime).c_str());

                ImGui::EndTable();
            }
        }
    }

    PointLight* light = getPointLight(scene, entityID);
    if (light != nullptr) {
        if (ImGui::CollapsingHeader("Point Light", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::BeginTable("Point Light Table", 2, ImGuiTableFlags_SizingFixedSame)) {
                ImGui::TableSetupColumn("##Label", ImGuiTableColumnFlags_None, 0.0f, 200.0f);
                ImGui::TableSetupColumn("##Widget", ImGuiTableColumnFlags_WidthStretch);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Brightness");
                ImGui::TableSetColumnIndex(1);
                ImGui::DragFloat("##brightness", &light->brightness, 0.01f, 0.0f, 10000.0f);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Color");
                ImGui::TableSetColumnIndex(1);
                ImGui::ColorEdit3("##color", light->color.mF32, ImGuiColorEditFlags_DisplayHSV | ImGuiColorEditFlags_PickerHueBar);
                ImGui::EndTable();
            }
        }
    }

    BoxCollider* collider = getBoxCollider(scene, entityID);
    if (collider != nullptr) {
    }

    SpotLight* spotLight = getSpotLight(scene, entityID);
    if (spotLight != nullptr) {
        if (ImGui::CollapsingHeader("Spot Light", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::BeginTable("Spot Light Table", 2, ImGuiTableFlags_SizingFixedSame)) {
                ImGui::TableSetupColumn("##Label", ImGuiTableColumnFlags_None, 0.0f, 200.0f);
                ImGui::TableSetupColumn("##Widget", ImGuiTableColumnFlags_WidthStretch);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Brightness");
                ImGui::TableSetColumnIndex(1);
                ImGui::DragFloat("##brightness", &spotLight->brightness, 0.01f, 0.0f, 100.0f);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Color");
                ImGui::TableSetColumnIndex(1);
                ImGui::ColorEdit3("##color", spotLight->color.mF32, ImGuiColorEditFlags_DisplayHSV | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_HDR);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Inner Cutoff");
                ImGui::TableSetColumnIndex(1);
                ImGui::DragFloat("##inner cutoff", &spotLight->cutoff, 0.01f, 0.0f, spotLight->outerCutoff - 0.01f);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Outer Cutoff");
                ImGui::TableSetColumnIndex(1);
                ImGui::DragFloat("##outer cutoff", &spotLight->outerCutoff, 0.01f, spotLight->cutoff + 0.01f, 180.0f);

                if (ImGui::CollapsingHeader("Shadow Map")) {
                    ImGui::Image((ImTextureID)(intptr_t)spotLight->depthTex, ImVec2(200, 200));
                }

                ImGui::EndTable();
            }
        }
    }

    MeshRenderer* renderer = getMeshRenderer(scene, entityID);
    if (renderer != nullptr) {
        if (ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::BeginTable("Mesh Renderer Table", 3, ImGuiTableFlags_SizingFixedSame)) {
                ImGui::TableSetupColumn("##Label", ImGuiTableColumnFlags_None, 0.0f, 100.0f);
                ImGui::TableSetupColumn("##Widget", ImGuiTableColumnFlags_None, 20.0f);
                ImGui::TableSetupColumn("##map", ImGuiTableColumnFlags_WidthStretch);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Mesh");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text(renderer->mesh->name.c_str());

                Material* material = renderer->mesh->subMeshes[0].material;
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Material");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text(material->name.c_str());

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Albedo");
                ImGui::TableSetColumnIndex(1);
                ImGui::Image((ImTextureID)(intptr_t)material->textures[0].id, ImVec2(20, 20));
                ImGui::TableSetColumnIndex(2);
                ImGui::ColorEdit4("##color", material->baseColor.mF32, ImGuiColorEditFlags_HDR);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Roughness");
                ImGui::TableSetColumnIndex(1);
                ImGui::Image((ImTextureID)(intptr_t)material->textures[1].id, ImVec2(20, 20));
                ImGui::TableSetColumnIndex(2);
                ImGui::DragFloat("##roughness", &material->roughness, 0.01f, 0.0f, 1.0f);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Metalness");
                ImGui::TableSetColumnIndex(1);
                ImGui::Image((ImTextureID)(intptr_t)material->textures[2].id, ImVec2(20, 20));
                ImGui::TableSetColumnIndex(2);
                ImGui::DragFloat("##metalness", &material->metalness, 0.01f, 0.0f, 1.0f);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("AO");
                ImGui::TableSetColumnIndex(1);
                ImGui::Image((ImTextureID)(intptr_t)material->textures[3].id, ImVec2(20, 20));
                ImGui::TableSetColumnIndex(2);
                ImGui::DragFloat("##ao", &material->aoStrength, 0.01f, 0.0f, 1.0f);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Normal");
                ImGui::TableSetColumnIndex(1);
                ImGui::Image((ImTextureID)(intptr_t)material->textures[4].id, ImVec2(20, 20));
                ImGui::TableSetColumnIndex(2);
                ImGui::DragFloat("##normal", &material->normalStrength, 0.01f, 0.0f, 100.0f);

                ImGui::EndTable();
            }
        }
    }

    if (ImGui::Button("Delete Entity", ImVec2(55, 35))) {
        destroyEntity(scene, entityID);
    }

    ImGui::End();

    ImGui::Begin("Project");
    ImGui::End();

    ImGui::Begin("Post-Process");
    ImGui::DragFloat("Exposure", &scene->exposure, 0.01f, 0, 1000.0f);
    ImGui::DragFloat("Bloom Threshold", &scene->bloomThreshold, 0.01f, 0, 100.0f);
    ImGui::DragFloat("Bloom Amount", &scene->bloomAmount, 0.01f, 0, 100.0f);
    ImGui::DragFloat("Ambient", &scene->ambient, 0.001f, 0, 100.0f);
    ImGui::DragFloat("SSAO radius", &scene->AORadius, 0.001f, 0.0f, 90.0f);
    ImGui::DragFloat("SSAO bias", &scene->AOBias, 0.001f, 0.0f, 25.0f);
    ImGui::DragFloat("SSAO power", &scene->AOPower, 0.001f, 0.0f, 50.0f);
    ImGui::End();
    /* ImGui::Text("FPS: %.0f / FrameTime: %.6f", scene->FPS, scene->frameTime);
    ImGui::SliderFloat("Move Speed", &player->moveSpeed, 0.0f, 45.0f);
    ImGui::InputFloat("jump height", &player->jumpHeight);
    ImGui::InputFloat("gravity", &scene->gravity);
    ImGui::DragFloat("Normal Strength", &scene->normalStrength, 0.01f, 0, 100.0f);
    ImGui::DragFloat("Exposure", &scene->exposure, 0.01f, 0, 1000.0f);
    ImGui::DragFloat("Bloom Threshold", &scene->bloomThreshold, 0.01f, 0, 100.0f);
    ImGui::DragFloat("Bloom Amount", &scene->bloomAmount, 0.01f, 0, 100.0f);
    ImGui::DragFloat("Ambient", &scene->ambient, 0.001f, 0, 100.0f);
    ImGui::DragFloat("SSAO radius", &scene->AORadius, 0.001f, 0.0f, 90.0f);
    ImGui::DragFloat("SSAO bias", &scene->AOBias, 0.001f, 0.0f, 25.0f);
    ImGui::DragFloat("SSAO power", &scene->AOPower, 0.001f, 0.0f, 50.0f);

    if (ImGui::Button("Save Scene", ImVec2(75, 40))) {
        saveScene(scene);
    }
    for (int i = 0; i < scene->transforms.size(); i++) {
        if (scene->transforms[i].parentEntityID == INVALID_ID) {
            createImGuiEntityTree(scene, scene->transforms[i].entityID, node_flags);
        }
    } */
}

void drawDebug(Scene* scene, ImGuiTreeNodeFlags nodeFlags, Player* player) {
    buildImGui(scene, nodeFlags, player);
    // ImGui::BeginMainMenuBar();
    // ImGui::Image((ImTextureID)(intptr_t)scene->editorTex, ImVec2(800, 600));
    // ImGui::EndMainMenuBar();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}