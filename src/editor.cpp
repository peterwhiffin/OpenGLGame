#include <iostream>
#include <filesystem>
#include <fstream>

#include "utils/imgui.h"
#include "utils/imgui_impl_glfw.h"
#include "utils/imgui_impl_opengl3.h"
#include "editor.h"
#include "scene.h"
#include "sceneloader.h"
#include "ecs.h"
#include "transform.h"
#include "renderer.h"
#include "physics.h"
#include "animation.h"

void createProjectTree(Scene* scene, ImGuiTreeNodeFlags node_flags, std::string directory);

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

void createEntityTree(Scene* scene, uint32_t entityID, ImGuiTreeNodeFlags node_flags, ImGuiSelectionBasicStorage& selection) {
    Entity* entity = getEntity(scene, entityID);
    Transform* transform = getTransform(scene, entityID);

    ImGui::PushID(entityID);

    node_flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;

    if (transform->childEntityIds.size() == 0) {
        node_flags |= ImGuiTreeNodeFlags_Leaf;
    }

    ImGui::SetNextItemSelectionUserData(entityID);

    bool is_selected = selection.Contains(entityID);
    if (is_selected) {
        node_flags |= ImGuiTreeNodeFlags_Selected;
    }

    std::string title = entity->name;
    bool node_open = ImGui::TreeNodeEx(title.c_str(), node_flags);
    if (ImGui::IsItemClicked()) {
        scene->nodeClicked = entity->entityID;
        scene->fileClicked = "";
    }

    if (node_open) {
        for (uint32_t childEntityID : transform->childEntityIds) {
            createEntityTree(scene, childEntityID, node_flags, selection);
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void buildImGui(Scene* scene) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

    if (scene->timeAccum >= 1.0f) {
        scene->FPS = scene->frameCount / scene->timeAccum;
        scene->frameTime = (scene->timeAccum / scene->frameCount) * 1000.0f;
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
    std::string fps = std::to_string(scene->FPS);
    std::string frameTime = std::to_string(scene->frameTime);
    fps = fps.substr(0, fps.find_first_of('.'));
    frameTime = frameTime.substr(0, frameTime.find_first_of('.') + 3);
    ImGui::Text(("FPS: " + fps + " / Frame Time: " + frameTime + "ms").c_str());
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

    static ImGuiSelectionBasicStorage selection;

    ImGui::Begin("SceneHierarchy");
    ImGuiMultiSelectFlags selectFlags = ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_BoxSelect1d;
    ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(selectFlags, selection.Size, scene->entities.size());
    selection.ApplyRequests(ms_io);

    for (int i = 0; i < scene->transforms.size(); i++) {
        if (scene->transforms[i].parentEntityID == INVALID_ID) {
            createEntityTree(scene, scene->transforms[i].entityID, 0, selection);
        }
    }

    ms_io = ImGui::EndMultiSelect();
    selection.ApplyRequests(ms_io);

    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        ImGui::OpenPopup("HierarchyContextMenu");
    }

    if (ImGui::BeginPopup("HierarchyContextMenu")) {
        if (ImGui::MenuItem("Create Entity")) {
            getNewEntity(scene, "NewEntity");
        }

        ImGui::EndPopup();
    }

    if (scene->input.deleteKey) {
        if (scene->canDelete) {
            scene->canDelete = false;

            for (int i = 0; i < selection._Storage.Data.Size; ++i) {
                ImGuiID key = selection._Storage.Data[i].key;
                if (selection._Storage.GetInt(key, 0) != 0) {
                    uint32_t entityID = static_cast<uint32_t>(key);
                    destroyEntity(scene, entityID);
                }
            }
        }
    } else {
        scene->canDelete = true;
    }

    unsigned int entityID = scene->nodeClicked;

    /* if (entityID != INVALID_ID) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            ImGui::OpenPopup("HierarchyItemContextMenu");
        }
        if (ImGui::BeginPopup("HierarchyItemContextMenu")) {
            if (ImGui::MenuItem(getEntity(scene, entityID)->name.c_str())) {
            }

            ImGui::EndPopup();
        }
    } */

    ImGui::End();

    ImGui::Begin("Project");

    createProjectTree(scene, 0, "..\\resources\\");

    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        ImGui::OpenPopup("ProjectContextMenu");
    }

    if (ImGui::BeginPopup("ProjectContextMenu")) {
        if (ImGui::MenuItem("Create Material")) {
            std::string fileName = "NewMaterial.mat";
            std::string name = "NewMaterial";
            std::string suffix = "";
            std::string ext = ".mat";
            int counter = 0;

            while (!checkFilenameUnique("..\\resources\\", fileName)) {
                counter++;
                suffix = std::to_string(counter);
                fileName = name + suffix + ext;
            }

            name = name + suffix;
            std::string textures = "white, white, white, white, white";
            std::string baseColor = "1.0, 1.0, 1.0, 1.0";
            std::string roughness = "1.0";
            std::string metalness = "1.0";
            std::string aoStrength = "1.0";
            std::string normalStrength = "1.0";
            std::string textureTiling = "1.0, 1.0";

            std::ofstream stream("..\\resources\\" + fileName);
            stream << "Material {" << std::endl;
            stream << "textures: " << textures << std::endl;
            stream << "textureTiling: " << textureTiling << std::endl;
            stream << "baseColor: " << baseColor << std::endl;
            stream << "roughness: " << roughness << std::endl;
            stream << "metalness: " << metalness << std::endl;
            stream << "aoStrength: " << aoStrength << std::endl;
            stream << "normalStrength: " << normalStrength << std::endl;
            stream << "}" << std::endl
                   << std::endl;

            Material* newMat = new Material();
            newMat->name = fileName;
            newMat->textures.push_back(scene->textureMap["white"]);
            newMat->textures.push_back(scene->textureMap["white"]);
            newMat->textures.push_back(scene->textureMap["black"]);
            newMat->textures.push_back(scene->textureMap["white"]);
            newMat->textures.push_back(scene->textureMap["blue"]);
            newMat->textureTiling = glm::vec2(1.0f, 1.0f);
            newMat->baseColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
            newMat->roughness = 1.0f;
            newMat->metalness = 1.0f;
            newMat->aoStrength = 1.0f;
            newMat->normalStrength = 1.0f;
            scene->materialMap[fileName] = newMat;
        }

        ImGui::EndPopup();
    }
    ImGui::End();
    ImGui::Begin("Inspector");

    if (entityID != INVALID_ID) {
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::BeginTable("Transform Table", 2, ImGuiTableFlags_SizingFixedSame)) {
                Transform* transform = getTransform(scene, entityID);
                vec3 position = transform->localPosition;
                vec3 worldPosition = transform->worldTransform.GetTranslation();
                vec3 rotation = getLocalRotation(scene, entityID).GetEulerAngles();
                vec3 degrees = vec3(JPH::RadiansToDegrees(rotation.GetX()), JPH::RadiansToDegrees(rotation.GetY()), JPH::RadiansToDegrees(rotation.GetZ()));
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
                ImGui::DragFloat3("##Position", position.mF32, 0.01f);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Rotation");
                ImGui::TableSetColumnIndex(1);
                ImGui::DragFloat3("##Rotation", degrees.mF32, 0.01f);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Scale");
                ImGui::TableSetColumnIndex(1);
                ImGui::DragFloat3("##Scale", scale.mF32, 0.1f);

                vec3 newPos;
                newPos.SetX(position.GetX());
                newPos.SetY(position.GetY());
                newPos.SetZ(position.GetZ());
                vec3 radians = vec3(JPH::DegreesToRadians(degrees.GetX()), JPH::DegreesToRadians(degrees.GetY()), JPH::DegreesToRadians(degrees.GetZ()));
                RigidBody* rb = getRigidbody(scene, entityID);
                if (rb != nullptr) {
                    scene->bodyInterface->SetPositionAndRotation(rb->joltBody, newPos, quat::sEulerAngles(radians), JPH::EActivation::Activate);
                    setLocalPosition(scene, entityID, position);
                    setLocalRotation(scene, entityID, quat::sEulerAngles(radians));
                    setLocalScale(scene, entityID, scale);
                } else {
                    setLocalPosition(scene, entityID, position);
                    setLocalRotation(scene, entityID, quat::sEulerAngles(radians));
                    setLocalScale(scene, entityID, scale);
                }

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
                    ImGui::ColorEdit3("##color", light->color.mF32, ImGuiColorEditFlags_DisplayHSV | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_HDR);
                    ImGui::EndTable();
                }
            }
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
                    ImGui::DragFloat("##brightness", &spotLight->brightness, 0.01f, 0.0f, 1000.0f);

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

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Range");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::DragFloat("##range", &spotLight->range, 0.01f, 0.0, 1000.0f);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Light Radius");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::DragFloat("##lightRadius", &spotLight->lightRadiusUV, 0.0001f, 0.0f, 180.0f);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Blocker Search");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::DragFloat("##blockerSearch", &spotLight->blockerSearchUV, 0.0001f, 0.00f, 180.0f);

                    if (ImGui::CollapsingHeader("Shadow Map")) {
                        ImGui::Image((ImTextureID)(intptr_t)spotLight->depthTex, ImVec2(200, 200));
                    }

                    ImGui::EndTable();
                }
            }
        }

        RigidBody* rigidbody = getRigidbody(scene, entityID);
        if (rigidbody != nullptr) {
            const JPH::Shape* shape = scene->bodyInterface->GetShape(rigidbody->joltBody).GetPtr();
            const JPH::BoxShape* box;
            const JPH::SphereShape* sphere;
            const JPH::CapsuleShape* capsule;
            const JPH::CylinderShape* cylinder;
            JPH::EShapeSubType shapeType = shape->GetSubType();
            JPH::Color color(0, 255, 0);
            JPH::AABox localBox;
            float radius = 0.5f;
            float halfHeight = 0.5f;

            switch (shapeType) {
                case JPH::EShapeSubType::Box:
                    box = static_cast<const JPH::BoxShape*>(shape);
                    vec3 halfExtents = box->GetHalfExtent();
                    localBox = JPH::AABox(-halfExtents, halfExtents);
                    scene->debugRenderer->DrawBox(scene->bodyInterface->GetWorldTransform(rigidbody->joltBody), localBox, color, JPH::DebugRenderer::ECastShadow::Off, JPH::DebugRenderer::EDrawMode::Wireframe);
                    break;
                case JPH::EShapeSubType::Sphere:
                    sphere = static_cast<const JPH::SphereShape*>(shape);
                    radius = sphere->GetRadius();
                    scene->debugRenderer->DrawSphere(scene->bodyInterface->GetPosition(rigidbody->joltBody), radius, color, JPH::DebugRenderer::ECastShadow::Off, JPH::DebugRenderer::EDrawMode::Wireframe);
                    break;
                case JPH::EShapeSubType::Capsule:
                    capsule = static_cast<const JPH::CapsuleShape*>(shape);
                    halfHeight = capsule->GetHalfHeightOfCylinder();
                    radius = capsule->GetRadius();
                    scene->debugRenderer->DrawCapsule(scene->bodyInterface->GetWorldTransform(rigidbody->joltBody), halfHeight, radius, color, JPH::DebugRenderer::ECastShadow::Off, JPH::DebugRenderer::EDrawMode::Wireframe);
                    break;
                case JPH::EShapeSubType::Cylinder:
                    cylinder = static_cast<const JPH::CylinderShape*>(shape);
                    halfHeight = cylinder->GetHalfHeight();
                    radius = cylinder->GetRadius();
                    scene->debugRenderer->DrawCylinder(scene->bodyInterface->GetWorldTransform(rigidbody->joltBody), halfHeight, radius, color, JPH::DebugRenderer::ECastShadow::Off, JPH::DebugRenderer::EDrawMode::Wireframe);
                    break;
            }

            if (ImGui::CollapsingHeader("Rigidbody", ImGuiTreeNodeFlags_DefaultOpen)) {
                /* if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                    ImGui::OpenPopup("RigidbodyContext");
                }
                if (ImGui::BeginPopup("Rigidbody Stuff")) {
                    ImGui::Text("Remove Rigidbody");
                    ImGui::EndPopup();
                } */

                if (ImGui::BeginTable("Rigidbody Table", 2, ImGuiTableFlags_SizingFixedSame)) {
                    const JPH::Shape* shape = scene->bodyInterface->GetShape(rigidbody->joltBody).GetPtr();
                    JPH::EShapeSubType shapeType = shape->GetSubType();
                    JPH::ObjectLayer objectLayer = scene->bodyInterface->GetObjectLayer(rigidbody->joltBody);
                    JPH::EMotionType motionType = scene->bodyInterface->GetMotionType(rigidbody->joltBody);
                    std::string motionTypeString = motionType == JPH::EMotionType::Dynamic ? "Dynamic" : "Static";
                    const JPH::BoxShape* box = static_cast<const JPH::BoxShape*>(shape);
                    vec3 halfExtents = box->GetHalfExtent();
                    ImGui::TableSetupColumn("##Label", ImGuiTableColumnFlags_None, 0.0f, 200.0f);
                    ImGui::TableSetupColumn("##Widget", ImGuiTableColumnFlags_WidthStretch);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Shape: ");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("Box");

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Motion Type: ");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text(motionTypeString.c_str());

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Half Extents: ");
                    ImGui::TableSetColumnIndex(1);
                    bool changed = ImGui::DragFloat3("##halfExtents", halfExtents.mF32, 0.01f, 0.0f, 0.0f);
                    ImGui::EndTable();

                    if (changed) {
                        if (shapeType == JPH::EShapeSubType::Box) {
                            vec3 newExtent = vec3(std::max(halfExtents.GetX(), 0.0f), std::max(halfExtents.GetY(), 0.0f), std::max(halfExtents.GetZ(), 0.0f));
                            JPH::BoxShapeSettings boxShapeSettings(newExtent);
                            JPH::ShapeSettings::ShapeResult boxResult = boxShapeSettings.Create();
                            JPH::ShapeRefC boxShape = boxResult.Get();
                            scene->bodyInterface->SetShape(rigidbody->joltBody, boxShape, false, JPH::EActivation::DontActivate);
                        }
                    }
                }
            }
        }

        MeshRenderer* renderer = getMeshRenderer(scene, entityID);
        if (renderer != nullptr) {
            bool isOpen = ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen);

            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                ImGui::OpenPopup("MeshRendererContextMenu");
            }
            if (ImGui::BeginPopup("MeshRendererContextMenu")) {
                if (ImGui::MenuItem("Remove")) {
                    destroyComponent(scene->meshRenderers, scene->meshRendererIndexMap, entityID);
                }

                ImGui::EndPopup();
            }

            if (isOpen) {
                if (ImGui::BeginTable("Mesh Renderer Table", 3, ImGuiTableFlags_SizingFixedFit)) {
                    ImGui::TableSetupColumn("##Label", ImGuiTableColumnFlags_WidthFixed, 75.0f);
                    ImGui::TableSetupColumn("##Widget", ImGuiTableColumnFlags_WidthFixed, 145.0f);
                    ImGui::TableSetupColumn("##map", ImGuiTableColumnFlags_WidthStretch);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Mesh");
                    ImGui::TableSetColumnIndex(1);
                    std::string meshName = renderer->mesh == nullptr ? "None" : renderer->mesh->name;
                    if (ImGui::BeginCombo("##currentMesh", meshName.c_str())) {
                        const bool isSelected = false;

                        for (auto& pair : scene->meshMap) {
                            if (ImGui::Selectable(pair.first.c_str(), isSelected)) {
                                renderer->mesh = pair.second;
                            }
                        }

                        ImGui::EndCombo();
                    }
                    // ImGui::Text(renderer->mesh->name.c_str());

                    if (renderer->mesh != nullptr) {
                        Material* material = renderer->mesh->subMeshes[0].material;

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Material");
                        ImGui::TableSetColumnIndex(1);
                        if (ImGui::BeginCombo("##currentMaterial", renderer->mesh->subMeshes[0].material->name.c_str())) {
                            const bool isSelected = false;

                            for (auto& pair : scene->materialMap) {
                                if (ImGui::Selectable(pair.first.c_str(), isSelected)) {
                                    renderer->mesh->subMeshes[0].material = pair.second;
                                }
                            }

                            ImGui::EndCombo();
                        }

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Albedo");
                        ImGui::TableSetColumnIndex(1);
                        if (ImGui::BeginCombo("##currentAlbedoMap", material->textures[0]->name.c_str())) {
                            const bool isSelected = false;

                            for (auto& pair : scene->textureMap) {
                                ImGui::Image((ImTextureID)(intptr_t)pair.second->id, ImVec2(20, 20));
                                ImGui::SameLine();
                                if (ImGui::Selectable(pair.first.c_str(), isSelected)) {
                                    material->textures[0] = pair.second;
                                }
                            }

                            ImGui::EndCombo();
                        }
                        ImGui::SameLine();
                        ImGui::Image((ImTextureID)(intptr_t)material->textures[0]->id, ImVec2(20, 20));
                        ImGui::TableSetColumnIndex(2);
                        ImGui::ColorEdit4("##color", material->baseColor.mF32, ImGuiColorEditFlags_HDR);

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Roughness");
                        ImGui::TableSetColumnIndex(1);
                        if (ImGui::BeginCombo("##currentRoughnessMap", material->textures[1]->name.c_str())) {
                            const bool isSelected = false;

                            for (auto& pair : scene->textureMap) {
                                ImGui::Image((ImTextureID)(intptr_t)pair.second->id, ImVec2(20, 20));
                                ImGui::SameLine();
                                if (ImGui::Selectable(pair.first.c_str(), isSelected)) {
                                    material->textures[1] = pair.second;
                                }
                            }

                            ImGui::EndCombo();
                        }
                        ImGui::SameLine();
                        ImGui::Image((ImTextureID)(intptr_t)material->textures[1]->id, ImVec2(20, 20));
                        ImGui::TableSetColumnIndex(2);
                        ImGui::DragFloat("##roughness", &material->roughness, 0.01f, 0.0f, 1.0f);

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Metalness");
                        ImGui::TableSetColumnIndex(1);
                        if (ImGui::BeginCombo("##currentMetallicMap", material->textures[2]->name.c_str())) {
                            const bool isSelected = false;

                            for (auto& pair : scene->textureMap) {
                                ImGui::Image((ImTextureID)(intptr_t)pair.second->id, ImVec2(20, 20));
                                ImGui::SameLine();
                                if (ImGui::Selectable(pair.first.c_str(), isSelected)) {
                                    material->textures[2] = pair.second;
                                }
                            }

                            ImGui::EndCombo();
                        }
                        ImGui::SameLine();
                        ImGui::Image((ImTextureID)(intptr_t)material->textures[2]->id, ImVec2(20, 20));
                        ImGui::TableSetColumnIndex(2);
                        ImGui::DragFloat("##metalness", &material->metalness, 0.01f, 0.0f, 1.0f);

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("AO");
                        ImGui::TableSetColumnIndex(1);
                        if (ImGui::BeginCombo("##currentAOMap", material->textures[3]->name.c_str())) {
                            const bool isSelected = false;

                            for (auto& pair : scene->textureMap) {
                                ImGui::Image((ImTextureID)(intptr_t)pair.second->id, ImVec2(20, 20));
                                ImGui::SameLine();
                                if (ImGui::Selectable(pair.first.c_str(), isSelected)) {
                                    material->textures[3] = pair.second;
                                }
                            }

                            ImGui::EndCombo();
                        }
                        ImGui::SameLine();
                        ImGui::Image((ImTextureID)(intptr_t)material->textures[3]->id, ImVec2(20, 20));
                        ImGui::TableSetColumnIndex(2);
                        ImGui::DragFloat("##ao", &material->aoStrength, 0.01f, 0.0f, 1.0f);

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Normal");
                        ImGui::TableSetColumnIndex(1);
                        if (ImGui::BeginCombo("##currentNormalMap", material->textures[4]->name.c_str())) {
                            const bool isSelected = false;

                            for (auto& pair : scene->textureMap) {
                                ImGui::Image((ImTextureID)(intptr_t)pair.second->id, ImVec2(20, 20));
                                ImGui::SameLine();
                                if (ImGui::Selectable(pair.first.c_str(), isSelected)) {
                                    material->textures[4] = pair.second;
                                }
                            }

                            ImGui::EndCombo();
                        }
                        ImGui::SameLine();
                        ImGui::Image((ImTextureID)(intptr_t)material->textures[4]->id, ImVec2(20, 20));
                        ImGui::TableSetColumnIndex(2);
                        ImGui::DragFloat("##normal", &material->normalStrength, 0.01f, 0.0f, 100.0f);

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Tiling");
                        ImGui::TableSetColumnIndex(1);
                        ImGui::DragFloat2("##tiling", glm::value_ptr(material->textureTiling), 0.001f);
                    }
                    ImGui::EndTable();
                }
            }
        }

        if (ImGui::BeginCombo("##Add Component", "Add Component")) {
            const bool isSelected = false;
            if (getMeshRenderer(scene, entityID) == nullptr) {
                if (ImGui::Selectable("Mesh Renderer", isSelected)) {
                    MeshRenderer* renderer = addMeshRenderer(scene, entityID);
                    renderer->mesh = nullptr;
                    uint32_t parentID = getTransform(scene, entityID)->parentEntityID;
                    uint32_t childID = entityID;
                    while (parentID != INVALID_ID) {
                        childID = parentID;
                        parentID = getTransform(scene, childID)->parentEntityID;
                    }

                    renderer->rootEntity = childID;
                }
            }

            if (getRigidbody(scene, entityID) == nullptr) {
                if (ImGui::Selectable("Rigidbody: Box", isSelected)) {
                    RigidBody* rb = addRigidbody(scene, entityID);
                    JPH::BoxShapeSettings boxSettings(vec3(0.5f, 0.5f, 0.5f));
                    JPH::ShapeSettings::ShapeResult shapeResult = boxSettings.Create();
                    JPH::ShapeRefC shape = shapeResult.Get();
                    JPH::BodyCreationSettings bodySettings(shape, JPH::RVec3(0.0_r, 0.0_r, 0.0_r), quat::sIdentity(), JPH::EMotionType::Static, Layers::NON_MOVING);
                    JPH::Body* body = scene->bodyInterface->CreateBody(bodySettings);
                    scene->bodyInterface->AddBody(body->GetID(), JPH::EActivation::DontActivate);
                    rb->joltBody = body->GetID();
                }
            }

            if (getSpotLight(scene, entityID) == nullptr) {
                if (ImGui::Selectable("Spot Light", isSelected)) {
                    SpotLight* light = addSpotLight(scene, entityID);
                    light->brightness = 10.0f;
                    light->color = vec3(1.0f, 1.0f, 1.0f);
                    light->cutoff = 1.0f;
                    light->outerCutoff = 15.0f;
                    createSpotLightShadowMap(scene, light);
                }
            }
            ImGui::EndCombo();
        }
    } else if (scene->fileClicked != "") {
        std::string extension = scene->fileClicked.substr(scene->fileClicked.find_last_of('.'));
        std::string fileName = scene->fileClicked.substr(scene->fileClicked.find_last_of('\\'));
        std::string name = fileName.substr(1);

        if (extension == ".png") {
            if (ImGui::BeginTable("PNG Import Table", 2, ImGuiTableFlags_SizingFixedSame)) {
                ImGui::TableSetupColumn("##Label", ImGuiTableColumnFlags_None, 0.0f, 200.0f);
                ImGui::TableSetupColumn("##Widget", ImGuiTableColumnFlags_WidthStretch);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Path: ");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text(scene->fileClicked.c_str());

                TextureSettings* settings = &scene->textureImportMap[scene->fileClicked];
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Gamma");
                ImGui::TableSetColumnIndex(1);
                ImGui::Checkbox("##gammaBool", &settings->gamma);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Filter ");
                ImGui::TableSetColumnIndex(1);
                GLenum filter = settings->filter;
                std::string filterString = "Nearest";
                if (filter == GL_LINEAR) {
                    filterString = "Linear";
                }

                if (ImGui::BeginCombo("##pixelFormat", filterString.c_str())) {
                    const bool isSelected = false;

                    if (ImGui::Selectable("Nearest", isSelected)) {
                        settings->filter = GL_NEAREST;
                    } else if (ImGui::Selectable("Linear", isSelected)) {
                        settings->filter = GL_LINEAR;
                    }

                    ImGui::EndCombo();
                }

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Image((ImTextureID)(intptr_t)scene->textureMap[name]->id, ImVec2(100, 100));

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                if (ImGui::Button("Apply", ImVec2(40.0f, 25.0f))) {
                    GLuint textureID = scene->textureMap[name]->id;  // Assume textureID is a valid texture object
                    GLint internalFormat;
                    glBindTexture(GL_TEXTURE_2D, textureID);
                    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);
                    if (settings->gamma) {
                        if (internalFormat == GL_RGB) {
                            settings->filter = GL_SRGB;
                        } else if (internalFormat == GL_RGBA) {
                            settings->filter = GL_SRGB_ALPHA;
                        }
                    } else {
                        if (internalFormat == GL_SRGB) {
                            settings->filter = GL_RGB;
                        } else if (internalFormat == GL_SRGB_ALPHA) {
                            settings->filter = GL_RGBA;
                        }
                    }

                    glDeleteTextures(1, &scene->textureMap[name]->id);
                    scene->textureMap[name]->id = loadTextureFromFile(settings->path.c_str(), *settings);

                    std::string gammaString = "true";
                    std::string filterString = "GL_NEAREST";

                    if (!settings->gamma) {
                        gammaString = "false";
                    }

                    if (settings->filter == GL_LINEAR) {
                        filterString = "GL_LINEAR";
                    }

                    std::ofstream stream(scene->fileClicked + ".meta");
                    stream << "Texture {" << std::endl;
                    stream << "path: " << scene->fileClicked << std::endl;
                    stream << "gamma: " << gammaString << std::endl;
                    stream << "filter: " << filterString << std::endl;
                    stream << "}" << std::endl
                           << std::endl;
                }
                ImGui::EndTable();
            }
        } else if (extension == ".mat") {
            if (ImGui::BeginTable("Material Table", 3, ImGuiTableFlags_SizingFixedFit)) {
                ImGui::TableSetupColumn("##Label", ImGuiTableColumnFlags_WidthFixed, 95.0f);
                ImGui::TableSetupColumn("##Widget", ImGuiTableColumnFlags_WidthFixed, 145.0f);
                ImGui::TableSetupColumn("##map", ImGuiTableColumnFlags_WidthStretch);

                // std::string matName = name.substr(0, name.find_last_of('.'));

                if (scene->materialMap.count(name)) {
                    Material* material = scene->materialMap[name];

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text(material->name.c_str());
                    static char buf1[32] = "";
                    ImGuiInputTextFlags flags;
                    flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ElideLeft;
                    ImGui::SameLine();

                    ImGui::TableSetColumnIndex(1);
                    if (ImGui::InputText("##defaultMatFilename", buf1, IM_ARRAYSIZE(buf1), flags)) {
                        std::filesystem::path path(scene->fileClicked);
                        if (std::filesystem::is_regular_file(path)) {
                            std::filesystem::path newPath = path;
                            newPath.replace_filename(buf1 + extension);
                            std::filesystem::rename(path, newPath);
                            scene->materialMap.erase(name);
                            material->name = buf1 + extension;
                            scene->materialMap[material->name] = material;
                            scene->fileClicked = newPath.string();
                            saveScene(scene);  // have to write the entire scene because mesh renderers get their material references by filename, and writing just the mesh renderers would be a disaster. Would need a stable handle to the material file, such as a guid, to skip this.
                        }
                    }

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Albedo");
                    ImGui::TableSetColumnIndex(1);
                    if (ImGui::BeginCombo("##currentAlbedoMap", material->textures[0]->name.c_str())) {
                        const bool isSelected = false;

                        for (auto& pair : scene->textureMap) {
                            ImGui::Image((ImTextureID)(intptr_t)pair.second->id, ImVec2(20, 20));
                            ImGui::SameLine();
                            if (ImGui::Selectable(pair.first.c_str(), isSelected)) {
                                material->textures[0] = pair.second;
                            }
                        }

                        ImGui::EndCombo();
                    }
                    ImGui::SameLine();
                    ImGui::Image((ImTextureID)(intptr_t)material->textures[0]->id, ImVec2(20, 20));
                    ImGui::TableSetColumnIndex(2);
                    ImGui::ColorEdit4("##color", material->baseColor.mF32, ImGuiColorEditFlags_HDR);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Roughness");
                    ImGui::TableSetColumnIndex(1);
                    if (ImGui::BeginCombo("##currentRoughnessMap", material->textures[1]->name.c_str())) {
                        const bool isSelected = false;

                        for (auto& pair : scene->textureMap) {
                            ImGui::Image((ImTextureID)(intptr_t)pair.second->id, ImVec2(20, 20));
                            ImGui::SameLine();
                            if (ImGui::Selectable(pair.first.c_str(), isSelected)) {
                                material->textures[1] = pair.second;
                            }
                        }

                        ImGui::EndCombo();
                    }
                    ImGui::SameLine();
                    ImGui::Image((ImTextureID)(intptr_t)material->textures[1]->id, ImVec2(20, 20));
                    ImGui::TableSetColumnIndex(2);
                    ImGui::DragFloat("##roughness", &material->roughness, 0.01f, 0.0f, 1.0f);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Metalness");
                    ImGui::TableSetColumnIndex(1);
                    if (ImGui::BeginCombo("##currentMetallicMap", material->textures[2]->name.c_str())) {
                        const bool isSelected = false;

                        for (auto& pair : scene->textureMap) {
                            ImGui::Image((ImTextureID)(intptr_t)pair.second->id, ImVec2(20, 20));
                            ImGui::SameLine();
                            if (ImGui::Selectable(pair.first.c_str(), isSelected)) {
                                material->textures[2] = pair.second;
                            }
                        }

                        ImGui::EndCombo();
                    }
                    ImGui::SameLine();
                    ImGui::Image((ImTextureID)(intptr_t)material->textures[2]->id, ImVec2(20, 20));
                    ImGui::TableSetColumnIndex(2);
                    ImGui::DragFloat("##metalness", &material->metalness, 0.01f, 0.0f, 1.0f);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("AO");
                    ImGui::TableSetColumnIndex(1);
                    if (ImGui::BeginCombo("##currentAOMap", material->textures[3]->name.c_str())) {
                        const bool isSelected = false;

                        for (auto& pair : scene->textureMap) {
                            ImGui::Image((ImTextureID)(intptr_t)pair.second->id, ImVec2(20, 20));
                            ImGui::SameLine();
                            if (ImGui::Selectable(pair.first.c_str(), isSelected)) {
                                material->textures[3] = pair.second;
                            }
                        }

                        ImGui::EndCombo();
                    }
                    ImGui::SameLine();
                    ImGui::Image((ImTextureID)(intptr_t)material->textures[3]->id, ImVec2(20, 20));
                    ImGui::TableSetColumnIndex(2);
                    ImGui::DragFloat("##ao", &material->aoStrength, 0.01f, 0.0f, 1.0f);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Normal");
                    ImGui::TableSetColumnIndex(1);
                    if (ImGui::BeginCombo("##currentNormalMap", material->textures[4]->name.c_str())) {
                        const bool isSelected = false;

                        for (auto& pair : scene->textureMap) {
                            ImGui::Image((ImTextureID)(intptr_t)pair.second->id, ImVec2(20, 20));
                            ImGui::SameLine();
                            if (ImGui::Selectable(pair.first.c_str(), isSelected)) {
                                material->textures[4] = pair.second;
                            }
                        }

                        ImGui::EndCombo();
                    }
                    ImGui::SameLine();
                    ImGui::Image((ImTextureID)(intptr_t)material->textures[4]->id, ImVec2(20, 20));
                    ImGui::TableSetColumnIndex(2);
                    ImGui::DragFloat("##normal", &material->normalStrength, 0.01f, 0.0f, 100.0f);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Tiling");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::DragFloat2("##tiling", glm::value_ptr(material->textureTiling), 0.001f);
                }
                ImGui::EndTable();
            }
        }
    }
    /* if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        ImGui::OpenPopup("InspectorContextMenu");
    }
    if (ImGui::BeginPopup("InspectorContextMenu")) {
        ImGui::Text("Inspector menu");
        ImGui::EndPopup();
    } */
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
}

bool checkFilenameUnique(std::string path, std::string filename) {
    for (const std::filesystem::directory_entry& dir : std::filesystem::directory_iterator(path)) {
        if (dir.is_regular_file()) {
            std::string fileString = dir.path().filename().string();
            if (fileString == filename) {
                return false;
            }
        }
    }

    return true;
}

void createProjectTree(Scene* scene, ImGuiTreeNodeFlags node_flags, std::string directory) {
    for (const std::filesystem::directory_entry& dir : std::filesystem::directory_iterator(directory)) {
        if (dir.is_directory()) {
            node_flags = 0;
            std::filesystem::path path = dir.path();
            std::string fullPath = dir.path().string();
            std::string name = fullPath.substr(fullPath.find_last_of('\\') + 1);
            ImGui::PushID(fullPath.c_str());
            bool isOpen = ImGui::TreeNodeEx(name.c_str(), node_flags);

            if (isOpen) {
                createProjectTree(scene, node_flags, fullPath);
                ImGui::TreePop();
            }
            ImGui::PopID();
        } else if (dir.is_regular_file()) {
            if (dir.path().extension() == ".meta") {
                continue;
            }
            std::string fileString = dir.path().filename().string();
            ImGui::PushID(fileString.c_str());

            node_flags = ImGuiTreeNodeFlags_Leaf;

            // ImGui::SetNextItemSelectionUserData(scene->fileClicked);
            if (scene->fileClicked == dir.path().string()) {
                node_flags |= ImGuiTreeNodeFlags_Selected;
            }

            ImGui::TreeNodeEx(fileString.c_str(), node_flags);

            if (ImGui::IsItemClicked()) {
                scene->fileClicked = dir.path().string();
                scene->nodeClicked = INVALID_ID;
            }

            ImGui::TreePop();
            ImGui::PopID();
        }
    }
}

void drawEditor(Scene* scene) {
    // drawPickingScene(scene);
    // checkPicker(scene, scene->input.cursorPosition);

    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    buildImGui(scene);
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void initEditor(Scene* scene) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    float aspectRatio = 1920.0f / 1080.0f;
    io.Fonts->AddFontFromFileTTF("../resources/fonts/Karla-Regular.ttf", aspectRatio * 8);
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(scene->window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    scene->debugRenderer = new MyDebugRenderer();
    JPH::DebugRenderer::sInstance = scene->debugRenderer;
    JPH::BodyManager::DrawSettings drawSettings;
    drawSettings.mDrawShape = true;
    drawSettings.mDrawShapeWireframe = true;
    drawSettings.mDrawShapeColor = JPH::BodyManager::EShapeColor::MotionTypeColor;

    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.12f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_InputTextCursor] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.35f, 0.58f, 0.86f);
    colors[ImGuiCol_TabSelected] = ImVec4(0.20f, 0.41f, 0.68f, 1.00f);
    colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_TabDimmed] = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
    colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
    colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextLink] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_TreeLines] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavCursor] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

void destroyEditor() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}