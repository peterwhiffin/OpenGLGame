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
#include "inspector.h"

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

    if (scene->entityIndexMap.count(id)) {
        scene->nodeClicked = id;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
        scene->inspectorState = SceneEntity;
        // scene->fileClicked = "";
    }

    if (node_open) {
        for (uint32_t childEntityID : transform->childEntityIds) {
            createEntityTree(scene, childEntityID, node_flags, selection);
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
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

            if (scene->fileClicked == dir.path().string()) {
                node_flags |= ImGuiTreeNodeFlags_Selected;
            }

            ImGui::TreeNodeEx(fileString.c_str(), node_flags);

            if (ImGui::IsItemClicked()) {
                scene->fileClicked = dir.path().string();
                scene->inspectorState = Resource;
                // scene->nodeClicked = INVALID_ID;
            }

            ImGui::TreePop();
            ImGui::PopID();
        }
    }
}

void ShowExampleMenuFile(Scene* scene) {
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

void buildMainMenu(Scene* scene) {
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
}

void buildSceneView(Scene* scene) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("EditViewport", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse);

    if (scene->timeAccum >= 1.0f) {
        scene->FPS = scene->frameCount / scene->timeAccum;
        scene->frameTime = (scene->timeAccum / scene->frameCount) * 1000.0f;
        scene->timeAccum = 0.0f;
        scene->frameCount = 0;
    } else {
        scene->timeAccum += scene->deltaTime;
        scene->frameCount++;
    }

    std::string fps = std::to_string(scene->FPS);
    std::string frameTime = std::to_string(scene->frameTime);
    fps = fps.substr(0, fps.find_first_of('.'));
    frameTime = frameTime.substr(0, frameTime.find_first_of('.') + 3);
    ImGui::Text(("FPS: " + fps + " / Frame Time: " + frameTime + "ms").c_str());
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
    ImGui::PopStyleVar();
}

void buildSceneHierarchy(Scene* scene) {
    ImGui::Begin("SceneHierarchy");
    static ImGuiSelectionBasicStorage selection;
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

    ImGui::End();
}

void buildProjectFiles(Scene* scene) {
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
}

void buildEnvironmentSettings(Scene* scene) {
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

void drawEditor(Scene* scene) {
    // drawPickingScene(scene);
    // checkPicker(scene, scene->input.cursorPosition);

    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
    ImGui::ShowDemoWindow(&scene->showDemoWindow);

    buildMainMenu(scene);
    buildSceneView(scene);
    buildSceneHierarchy(scene);
    buildProjectFiles(scene);
    buildInspector(scene);
    buildEnvironmentSettings(scene);

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