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

void createProjectTree(Scene* scene, EditorState* editor, ImGuiTreeNodeFlags node_flags, std::string directory);
void ExportImGuiStyleSizes();

void checkPicker(Scene* scene, RenderState* renderer, EditorState* editor, glm::dvec2 pickPosition) {
    if (!editor->isPicking) {
        return;
    }

    editor->isPicking = false;
    unsigned char pixel[3];
    glBindFramebuffer(GL_FRAMEBUFFER, renderer->pickingFBO);
    glReadPixels(pickPosition.x, renderer->windowData.height - pickPosition.y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
    uint32_t id = pixel[0] + (pixel[1] << 8) + (pixel[2] << 16);

    if (scene->entityIndexMap.count(id)) {
        editor->nodeClicked = id;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void createEntityTree(Scene* scene, EditorState* editor, uint32_t entityID, ImGuiTreeNodeFlags node_flags, ImGuiSelectionBasicStorage& selection) {
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
        editor->nodeClicked = entity->entityID;
        editor->inspectorState = SceneEntity;
    }

    if (node_open) {
        for (uint32_t childEntityID : transform->childEntityIds) {
            createEntityTree(scene, editor, childEntityID, node_flags, selection);
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void createProjectTree(Scene* scene, EditorState* editor, ImGuiTreeNodeFlags node_flags, std::string directory) {
    for (const std::filesystem::directory_entry& dir : std::filesystem::directory_iterator(directory)) {
        if (dir.is_directory()) {
            node_flags = 0;
            std::filesystem::path path = dir.path();
            std::string fullPath = dir.path().string();
            std::string name = fullPath.substr(fullPath.find_last_of('\\') + 1);
            ImGui::PushID(fullPath.c_str());
            bool isOpen = ImGui::TreeNodeEx(name.c_str(), node_flags);

            if (isOpen) {
                createProjectTree(scene, editor, node_flags, fullPath);
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

            if (editor->fileClicked == dir.path().string()) {
                node_flags |= ImGuiTreeNodeFlags_Selected;
            }

            ImGui::TreeNodeEx(fileString.c_str(), node_flags);

            if (ImGui::IsItemClicked()) {
                editor->fileClicked = dir.path().string();
                editor->inspectorState = Resource;
                // scene->nodeClicked = INVALID_ID;
            }

            ImGui::TreePop();
            ImGui::PopID();
        }
    }
}

void ShowExampleMenuFile(Scene* scene, Resources* resources, EditorState* editor) {
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
                ShowExampleMenuFile(scene, resources, editor);
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }

    static bool enabled = true;
    if (ImGui::MenuItem("Save", "Ctrl+S", &enabled, editor->mode == Edit)) {
        if (editor->mode == Edit) {
            saveScene(scene, resources);
        }
        // ExportImGuiStyleSizes();
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

void buildMainMenu(Scene* scene, Resources* resources, EditorState* editor) {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ShowExampleMenuFile(scene, resources, editor);
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

void buildSceneView(Scene* scene, RenderState* renderer, EditorState* editor, Resources* resources) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("EditViewport", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse);

    if (editor->timeAccum >= 1.0f) {
        editor->FPS = editor->frameCount / editor->timeAccum;
        editor->frameTime = (editor->timeAccum / editor->frameCount) * 1000.0f;
        editor->timeAccum = 0.0f;
        editor->frameCount = 0;
    } else {
        editor->timeAccum += scene->deltaTime;
        editor->frameCount++;
    }

    std::string fps = std::to_string(editor->FPS);
    std::string frameTime = std::to_string(editor->frameTime);
    fps = fps.substr(0, fps.find_first_of('.'));
    frameTime = frameTime.substr(0, frameTime.find_first_of('.') + 3);
    ImGui::Text(("FPS: " + fps + " / Frame Time: " + frameTime + "ms").c_str());
    ImGui::SameLine();

    if (editor->mode == Edit) {
        if (ImGui::Button("Play", ImVec2(20, 20))) {
            editor->mode = Play;
        }
    } else if (editor->mode = Play) {
        if (ImGui::Button("Stop", ImVec2(20, 20))) {
            editor->mode = Edit;
            clearScene(scene);
            loadScene(resources, scene);
        }
    }
    ImGui::Separator();
    ImVec2 availableSize = ImGui::GetContentRegionAvail();
    float aspectRatio = (float)renderer->windowData.viewportWidth / renderer->windowData.viewportHeight;

    ImVec2 imageSize = ImVec2(renderer->windowData.viewportWidth, renderer->windowData.viewportHeight);
    imageSize.y = glm::clamp(imageSize.y, 0.0f, availableSize.y);
    imageSize.x = imageSize.y * aspectRatio;

    if (imageSize.x > availableSize.x) {
        imageSize.x = availableSize.x;
        imageSize.y = imageSize.x * (1 / aspectRatio);
    }

    ImGui::SetCursorPos(ImVec2((availableSize.x - imageSize.x) / 2, (availableSize.y - imageSize.y) / 2));
    ImGui::Image((ImTextureID)(intptr_t)renderer->editorTex, imageSize, ImVec2(0, 1), ImVec2(1, 0));
    ImGui::End();
    ImGui::PopStyleVar();
}

void buildSceneHierarchy(Scene* scene, EditorState* editor) {
    ImGui::Begin("SceneHierarchy");
    static ImGuiSelectionBasicStorage selection;
    ImGuiMultiSelectFlags selectFlags = ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_BoxSelect1d;
    ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(selectFlags, selection.Size, scene->entities.size());
    selection.ApplyRequests(ms_io);

    for (int i = 0; i < scene->transforms.size(); i++) {
        if (scene->transforms[i].parentEntityID == INVALID_ID) {
            createEntityTree(scene, editor, scene->transforms[i].entityID, 0, selection);
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

    if (scene->input->deleteKey) {
        if (editor->canDelete) {
            editor->canDelete = false;

            for (int i = 0; i < selection._Storage.Data.Size; ++i) {
                ImGuiID key = selection._Storage.Data[i].key;
                if (selection._Storage.GetInt(key, 0) != 0) {
                    uint32_t entityID = static_cast<uint32_t>(key);
                    destroyEntity(scene, entityID);
                }
            }
        }
    } else {
        editor->canDelete = true;
    }

    ImGui::End();
}

void buildProjectFiles(Scene* scene, Resources* resources, EditorState* editor) {
    ImGui::Begin("Project");

    createProjectTree(scene, editor, 0, "..\\resources\\");

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

            Material* newMat = new Material();
            newMat->name = fileName;
            newMat->textures.push_back(resources->textureMap["white"]);
            newMat->textures.push_back(resources->textureMap["white"]);
            newMat->textures.push_back(resources->textureMap["black"]);
            newMat->textures.push_back(resources->textureMap["white"]);
            newMat->textures.push_back(resources->textureMap["blue"]);
            newMat->textureTiling = glm::vec2(1.0f, 1.0f);
            newMat->baseColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
            newMat->roughness = 1.0f;
            newMat->metalness = 1.0f;
            newMat->aoStrength = 1.0f;
            newMat->normalStrength = 1.0f;
            resources->materialMap[fileName] = newMat;
            writeMaterial(resources, "..\\resources\\" + fileName);
        }

        ImGui::EndPopup();
    }
    ImGui::End();
}

void buildEnvironmentSettings(RenderState* scene) {
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

void drawEditor(Scene* scene, RenderState* renderer, Resources* resources, EditorState* editor) {
    // drawPickingScene(scene);
    // checkPicker(scene, scene->input.cursorPosition);

    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
    ImGui::ShowDemoWindow(&editor->showDemoWindow);

    buildMainMenu(scene, resources, editor);
    buildSceneView(scene, renderer, editor, resources);
    buildSceneHierarchy(scene, editor);
    buildProjectFiles(scene, resources, editor);
    buildInspector(scene, resources, renderer, editor);
    buildEnvironmentSettings(renderer);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void initEditor(EditorState* editor, GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    float aspectRatio = 1920.0f / 1080.0f;
    io.Fonts->AddFontFromFileTTF("../resources/fonts/Karla-Regular.ttf", aspectRatio * 8);
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    JPH::BodyManager::DrawSettings drawSettings;
    drawSettings.mDrawShape = true;
    drawSettings.mDrawShapeWireframe = true;
    drawSettings.mDrawShapeColor = JPH::BodyManager::EShapeColor::MotionTypeColor;

    ImGuiStyle style = ImGui::GetStyle();
    style.Alpha = 1.00f;
    style.DisabledAlpha = 0.60f;
    style.WindowPadding = ImVec2(7.00f, 8.00f);
    style.WindowRounding = 0.00f;
    style.WindowBorderSize = 0.00f;
    style.WindowMinSize = ImVec2(32.00f, 32.00f);
    style.WindowTitleAlign = ImVec2(0.00f, 0.50f);
    style.ChildRounding = 0.00f;
    style.ChildBorderSize = 0.00f;
    style.PopupRounding = 0.00f;
    style.PopupBorderSize = 0.00f;
    style.FramePadding = ImVec2(4.00f, 3.00f);
    style.FrameRounding = 0.00f;
    style.FrameBorderSize = 1.00f;
    style.ItemSpacing = ImVec2(4.00f, 4.00f);
    style.ItemInnerSpacing = ImVec2(3.00f, 4.00f);
    style.IndentSpacing = 21.00f;
    style.CellPadding = ImVec2(6.00f, 1.00f);
    style.ScrollbarSize = 9.00f;
    style.ScrollbarRounding = 0.00f;
    style.GrabMinSize = 18.00f;
    style.GrabRounding = 0.00f;
    style.TabRounding = 0.15f;
    style.TabBorderSize = 0.00f;
    style.TabCloseButtonMinWidthSelected = 18.00f;
    style.TabCloseButtonMinWidthUnselected = 1.00f;
    style.DisplayWindowPadding = ImVec2(0.00f, 14.00f);
    style.DisplaySafeAreaPadding = ImVec2(7.00f, 3.00f);
    style.MouseCursorScale = 1.00f;
    style.AntiAliasedLines = true;
    style.AntiAliasedLinesUseTex = true;
    style.AntiAliasedFill = true;
    style.CurveTessellationTol = 1.25f;
    style.CircleTessellationMaxError = 0.30f;

    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.14f, 0.14f, 0.14f, 0.66f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.56f, 1.00f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.11f, 0.61f, 0.97f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.27f, 0.80f, 0.98f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.00f, 0.39f, 1.00f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 0.77f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.42f, 0.42f, 0.42f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.77f, 0.77f, 0.77f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.23f, 0.48f, 1.00f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.41f, 0.46f, 1.00f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.19f, 0.39f, 0.80f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.56f, 0.98f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.13f, 0.66f, 1.00f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_InputTextCursor] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.17f, 0.59f, 0.99f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TabSelected] = ImVec4(0.28f, 0.67f, 1.00f, 1.00f);
    colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_TabDimmed] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
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
    colors[ImGuiCol_TableRowBg] = ImVec4(1.00f, 0.24f, 0.24f, 1.00f);
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

void ExportImGuiStyleSizes() {
    ImGuiStyle& style = ImGui::GetStyle();
    printf("ImGuiStyle style = ImGui::GetStyle();\n");

    printf("style.Alpha = %.2ff;\n", style.Alpha);
    printf("style.DisabledAlpha = %.2ff;\n", style.DisabledAlpha);
    printf("style.WindowPadding = ImVec2(%.2ff, %.2ff);\n", style.WindowPadding.x, style.WindowPadding.y);
    printf("style.WindowRounding = %.2ff;\n", style.WindowRounding);
    printf("style.WindowBorderSize = %.2ff;\n", style.WindowBorderSize);
    printf("style.WindowMinSize = ImVec2(%.2ff, %.2ff);\n", style.WindowMinSize.x, style.WindowMinSize.y);
    printf("style.WindowTitleAlign = ImVec2(%.2ff, %.2ff);\n", style.WindowTitleAlign.x, style.WindowTitleAlign.y);
    printf("style.ChildRounding = %.2ff;\n", style.ChildRounding);
    printf("style.ChildBorderSize = %.2ff;\n", style.ChildBorderSize);
    printf("style.PopupRounding = %.2ff;\n", style.PopupRounding);
    printf("style.PopupBorderSize = %.2ff;\n", style.PopupBorderSize);
    printf("style.FramePadding = ImVec2(%.2ff, %.2ff);\n", style.FramePadding.x, style.FramePadding.y);
    printf("style.FrameRounding = %.2ff;\n", style.FrameRounding);
    printf("style.FrameBorderSize = %.2ff;\n", style.FrameBorderSize);
    printf("style.ItemSpacing = ImVec2(%.2ff, %.2ff);\n", style.ItemSpacing.x, style.ItemSpacing.y);
    printf("style.ItemInnerSpacing = ImVec2(%.2ff, %.2ff);\n", style.ItemInnerSpacing.x, style.ItemInnerSpacing.y);
    printf("style.IndentSpacing = %.2ff;\n", style.IndentSpacing);
    printf("style.CellPadding = ImVec2(%.2ff, %.2ff);\n", style.CellPadding.x, style.CellPadding.y);
    printf("style.ScrollbarSize = %.2ff;\n", style.ScrollbarSize);
    printf("style.ScrollbarRounding = %.2ff;\n", style.ScrollbarRounding);
    printf("style.GrabMinSize = %.2ff;\n", style.GrabMinSize);
    printf("style.GrabRounding = %.2ff;\n", style.GrabRounding);
    printf("style.TabRounding = %.2ff;\n", style.TabRounding);
    printf("style.TabBorderSize = %.2ff;\n", style.TabBorderSize);
    printf("style.TabCloseButtonMinWidthSelected = %.2ff;\n", style.TabCloseButtonMinWidthSelected);
    printf("style.TabCloseButtonMinWidthUnselected = %.2ff;\n", style.TabCloseButtonMinWidthUnselected);
    printf("style.DisplayWindowPadding = ImVec2(%.2ff, %.2ff);\n", style.DisplayWindowPadding.x, style.DisplayWindowPadding.y);
    printf("style.DisplaySafeAreaPadding = ImVec2(%.2ff, %.2ff);\n", style.DisplaySafeAreaPadding.x, style.DisplaySafeAreaPadding.y);
    printf("style.MouseCursorScale = %.2ff;\n", style.MouseCursorScale);
    printf("style.AntiAliasedLines = %s;\n", style.AntiAliasedLines ? "true" : "false");
    printf("style.AntiAliasedLinesUseTex = %s;\n", style.AntiAliasedLinesUseTex ? "true" : "false");
    printf("style.AntiAliasedFill = %s;\n", style.AntiAliasedFill ? "true" : "false");
    printf("style.CurveTessellationTol = %.2ff;\n", style.CurveTessellationTol);
    printf("style.CircleTessellationMaxError = %.2ff;\n", style.CircleTessellationMaxError);
}

void destroyEditor() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}