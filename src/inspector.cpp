#include <iostream>
#include <filesystem>
#include <fstream>

#include "utils/imgui.h"
#include "utils/imgui_impl_glfw.h"
#include "utils/imgui_impl_opengl3.h"
#include "inspector.h"
#include "editor.h"
#include "scene.h"
#include "sceneloader.h"
#include "ecs.h"
#include "transform.h"
#include "renderer.h"
#include "physics.h"
#include "animation.h"

void buildTextRow(std::string label, std::string value) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text(label.c_str());
    ImGui::TableSetColumnIndex(1);
    ImGui::Text(value.c_str());
}

void buildBoolRow(std::string label, bool* value) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text(label.c_str());
    ImGui::TableSetColumnIndex(1);
    ImGui::Checkbox(("##" + label).c_str(), value);
}

void buildFloatRow(std::string label, float* value, float speed = 0.01f, float min = 0.0f, float max = 0.0f) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text(label.c_str());
    ImGui::TableSetColumnIndex(1);
    ImGui::DragFloat(("##" + label).c_str(), value, speed, min, max);
}

void buildFloat2Row(std::string label, float* value, float speed = 0.01f, float min = 0.0f, float max = 0.0f) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text(label.c_str());
    ImGui::TableSetColumnIndex(1);
    ImGui::DragFloat2(("##" + label).c_str(), value, speed, min, max);
}

bool buildFloat3Row(std::string label, float* value, float speed = 0.01f, float min = 0.0f, float max = 0.0f) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text(label.c_str());
    ImGui::TableSetColumnIndex(1);
    return ImGui::DragFloat3(("##" + label).c_str(), value, speed, min, max);
}

void buildColor3Row(std::string label, float* value, ImGuiColorEditFlags flags) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Color");
    ImGui::TableSetColumnIndex(1);
    ImGui::ColorEdit3(("##" + label).c_str(), value, flags);
}

void buildColor4Row(std::string label, float* value, ImGuiColorEditFlags flags) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Color");
    ImGui::TableSetColumnIndex(1);
    ImGui::ColorEdit4(("##" + label).c_str(), value, flags);
}

void buildTextureMapRow(Resources* resources, std::string label, Texture* tex, float* value, bool color = false) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text(label.c_str());
    ImGui::TableSetColumnIndex(1);
    if (ImGui::BeginCombo(("##" + label).c_str(), tex->name.c_str())) {
        const bool isSelected = false;

        for (auto& pair : resources->textureMap) {
            ImGui::Image((ImTextureID)(intptr_t)pair.second->id, ImVec2(20, 20));
            ImGui::SameLine();
            if (ImGui::Selectable(pair.first.c_str(), isSelected)) {
                tex = pair.second;
            }
        }

        ImGui::EndCombo();
    }
    ImGui::SameLine();
    ImGui::Image((ImTextureID)(intptr_t)tex->id, ImVec2(20, 20));

    if (color) {
        ImGui::TableSetColumnIndex(2);
        ImGui::ColorEdit4("##color", value, ImGuiColorEditFlags_HDR);
    } else {
        ImGui::TableSetColumnIndex(2);
        ImGui::DragFloat("##roughness", value, 0.01f, 0.0f, 1.0f);
    }
}

void buildMaterialInspector(Resources* resources, Material* material) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Material");
    ImGui::TableSetColumnIndex(1);
    if (ImGui::BeginCombo("##currentMaterial", material->name.c_str())) {
        const bool isSelected = false;

        for (auto& pair : resources->materialMap) {
            if (ImGui::Selectable(pair.first.c_str(), isSelected)) {
                material = pair.second;
            }
        }

        ImGui::EndCombo();
    }

    buildTextureMapRow(resources, "Albedo", material->textures[0], material->baseColor.mF32, true);
    buildTextureMapRow(resources, "Roughness", material->textures[1], &material->roughness);
    buildTextureMapRow(resources, "Metalness", material->textures[2], &material->metalness);
    buildTextureMapRow(resources, "AO", material->textures[3], &material->aoStrength);
    buildTextureMapRow(resources, "Normal", material->textures[4], &material->normalStrength);
    buildFloat2Row("Tiling", glm::value_ptr(material->textureTiling), 0.001f);
}

void buildTransformInspector(Scene* scene, Transform* transform) {
    uint32_t entityID = transform->entityID;
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

            buildFloat3Row("World Position", worldPosition.mF32, 0.1f);
            buildFloat3Row("Position", position.mF32);
            buildFloat3Row("Rotation", degrees.mF32);
            buildFloat3Row("Scale", scale.mF32);

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
}

void buildMeshRendererInspector(Scene* scene, Resources* resources, MeshRenderer* renderer) {
    uint32_t entityID = renderer->entityID;
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

                for (auto& pair : resources->meshMap) {
                    if (ImGui::Selectable(pair.first.c_str(), isSelected)) {
                        renderer->mesh = pair.second;
                    }
                }

                ImGui::EndCombo();
            }

            if (renderer->mesh != nullptr) {
                buildMaterialInspector(resources, renderer->mesh->subMeshes[0].material);
            }

            ImGui::EndTable();
        }
    }
}

void buildAnimatorInspector(Animator* animator) {
    if (ImGui::CollapsingHeader("Animator", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable("Animator Table", 2, ImGuiTableFlags_SizingFixedSame)) {
            ImGui::TableSetupColumn("##Label", ImGuiTableColumnFlags_None, 0.0f, 200.0f);
            ImGui::TableSetupColumn("##Widget", ImGuiTableColumnFlags_WidthStretch);

            buildTextRow("Current Animation:", animator->currentAnimation->name);
            buildTextRow("Playback Time:", std::to_string(animator->playbackTime));

            ImGui::EndTable();
        }
    }
}

void buildRigidbodyInspector(Scene* scene, RenderState* renderer, EditorState* editor, RigidBody* rigidbody) {
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
            renderer->debugRenderer->DrawBox(scene->bodyInterface->GetWorldTransform(rigidbody->joltBody), localBox, color, JPH::DebugRenderer::ECastShadow::Off, JPH::DebugRenderer::EDrawMode::Wireframe);
            break;
        case JPH::EShapeSubType::Sphere:
            sphere = static_cast<const JPH::SphereShape*>(shape);
            radius = sphere->GetRadius();
            renderer->debugRenderer->DrawSphere(scene->bodyInterface->GetPosition(rigidbody->joltBody), radius, color, JPH::DebugRenderer::ECastShadow::Off, JPH::DebugRenderer::EDrawMode::Wireframe);
            break;
        case JPH::EShapeSubType::Capsule:
            capsule = static_cast<const JPH::CapsuleShape*>(shape);
            halfHeight = capsule->GetHalfHeightOfCylinder();
            radius = capsule->GetRadius();
            renderer->debugRenderer->DrawCapsule(scene->bodyInterface->GetWorldTransform(rigidbody->joltBody), halfHeight, radius, color, JPH::DebugRenderer::ECastShadow::Off, JPH::DebugRenderer::EDrawMode::Wireframe);
            break;
        case JPH::EShapeSubType::Cylinder:
            cylinder = static_cast<const JPH::CylinderShape*>(shape);
            halfHeight = cylinder->GetHalfHeight();
            radius = cylinder->GetRadius();
            renderer->debugRenderer->DrawCylinder(scene->bodyInterface->GetWorldTransform(rigidbody->joltBody), halfHeight, radius, color, JPH::DebugRenderer::ECastShadow::Off, JPH::DebugRenderer::EDrawMode::Wireframe);
            break;
    }

    if (ImGui::CollapsingHeader("Rigidbody", ImGuiTreeNodeFlags_DefaultOpen)) {
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

            buildTextRow("Shape: ", "Box");
            buildTextRow("Motion Type: ", motionTypeString);

            if (buildFloat3Row("Half Extents: ", halfExtents.mF32)) {
                if (shapeType == JPH::EShapeSubType::Box) {
                    vec3 newExtent = vec3(std::max(halfExtents.GetX(), 0.0f), std::max(halfExtents.GetY(), 0.0f), std::max(halfExtents.GetZ(), 0.0f));
                    JPH::BoxShapeSettings boxShapeSettings(newExtent);
                    JPH::ShapeSettings::ShapeResult boxResult = boxShapeSettings.Create();
                    JPH::ShapeRefC boxShape = boxResult.Get();
                    scene->bodyInterface->SetShape(rigidbody->joltBody, boxShape, false, JPH::EActivation::DontActivate);
                }
            }

            ImGui::EndTable();
        }
    }
}

void buildPointLightInspector(PointLight* light) {
    if (ImGui::CollapsingHeader("Point Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable("Point Light Table", 2, ImGuiTableFlags_SizingFixedSame)) {
            ImGui::TableSetupColumn("##Label", ImGuiTableColumnFlags_None, 0.0f, 200.0f);
            ImGui::TableSetupColumn("##Widget", ImGuiTableColumnFlags_WidthStretch);

            buildFloatRow("Brightness", &light->brightness);
            buildColor3Row("Color", light->color.mF32, ImGuiColorEditFlags_DisplayHSV | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_HDR);
            ImGui::EndTable();
        }
    }
}

void buildSpotLightInspector(SpotLight* spotLight) {
    if (ImGui::CollapsingHeader("Spot Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable("Spot Light Table", 2, ImGuiTableFlags_SizingFixedSame)) {
            ImGui::TableSetupColumn("##Label", ImGuiTableColumnFlags_None, 0.0f, 200.0f);
            ImGui::TableSetupColumn("##Widget", ImGuiTableColumnFlags_WidthStretch);

            buildFloatRow("Brightness", &spotLight->brightness);
            buildColor3Row("Color", spotLight->color.mF32, ImGuiColorEditFlags_DisplayHSV | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_HDR);
            buildFloatRow("Inner Angle", &spotLight->cutoff, 0.01f, 0.0f, spotLight->outerCutoff - 0.01f);
            buildFloatRow("Outer Angle", &spotLight->outerCutoff, 0.01f, spotLight->cutoff + 0.01f, 180.0f);
            buildFloatRow("Range", &spotLight->range);
            buildFloatRow("Light Radius UV", &spotLight->lightRadiusUV, 0.0001f, 0.0f, 180.0f);
            buildFloatRow("Blocker Search UV", &spotLight->blockerSearchUV, 0.0001f, 0.0f, 180.0f);

            if (ImGui::CollapsingHeader("Shadow Map")) {
                ImGui::Image((ImTextureID)(intptr_t)spotLight->depthTex, ImVec2(200, 200));
            }

            ImGui::EndTable();
        }
    }
}

void buildTextureInspector(Resources* resources, EditorState* editor) {
    std::string extension = editor->fileClicked.substr(editor->fileClicked.find_last_of('.'));
    std::string fileName = editor->fileClicked.substr(editor->fileClicked.find_last_of('\\'));
    std::string name = fileName.substr(1);
    TextureSettings* settings = &resources->textureImportMap[editor->fileClicked];

    if (ImGui::BeginTable("Texture Import Table", 2, ImGuiTableFlags_SizingFixedSame)) {
        ImGui::TableSetupColumn("##Label", ImGuiTableColumnFlags_None, 0.0f, 200.0f);
        ImGui::TableSetupColumn("##Widget", ImGuiTableColumnFlags_WidthStretch);

        buildTextRow("Path: ", editor->fileClicked.c_str());
        buildBoolRow("Gamma", &settings->gamma);

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
        ImGui::Image((ImTextureID)(intptr_t)resources->textureMap[name]->id, ImVec2(100, 100));

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        if (ImGui::Button("Apply", ImVec2(40.0f, 25.0f))) {
            glDeleteTextures(1, &resources->textureMap[name]->id);
            resources->textureMap[name]->id = loadTextureFromFile(settings->path.c_str(), *settings);
            writeTextureSettings(*settings);
        }
        ImGui::EndTable();
    }
}

void buildAddComponentCombo(Scene* scene, EditorState* editor) {
    if (ImGui::BeginCombo("##Add Component", "Add Component")) {
        const bool isSelected = false;
        uint32_t entityID = editor->nodeClicked;
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
}

void buildSceneEntityInspector(Scene* scene, RenderState* renderer, Resources* resources, EditorState* editor) {
    uint32_t entityID = editor->nodeClicked;
    Transform* transform = getTransform(scene, entityID);
    MeshRenderer* meshRenderer = getMeshRenderer(scene, entityID);
    Animator* animator = getAnimator(scene, entityID);
    RigidBody* rigidbody = getRigidbody(scene, entityID);
    SpotLight* spotLight = getSpotLight(scene, entityID);
    PointLight* pointLight = getPointLight(scene, entityID);

    buildTransformInspector(scene, transform);

    if (animator != nullptr) {
        buildAnimatorInspector(animator);
    }
    if (pointLight != nullptr) {
        buildPointLightInspector(pointLight);
    }

    if (spotLight != nullptr) {
        buildSpotLightInspector(spotLight);
    }

    if (rigidbody != nullptr) {
        buildRigidbodyInspector(scene, renderer, editor, rigidbody);
    }

    if (meshRenderer != nullptr) {
        buildMeshRendererInspector(scene, resources, meshRenderer);
    }

    buildAddComponentCombo(scene, editor);
}

void buildPrefabInspector(Scene* scene) {
}

void buildResourceInspector(Scene* scene, Resources* resources, EditorState* editor) {
    std::string extension = editor->fileClicked.substr(editor->fileClicked.find_last_of('.'));
    std::string fileName = editor->fileClicked.substr(editor->fileClicked.find_last_of('\\'));
    std::string name = fileName.substr(1);

    if (extension == ".png") {
        buildTextureInspector(resources, editor);
    } else if (extension == ".mat") {
        if (ImGui::BeginTable("Material Table", 3, ImGuiTableFlags_SizingFixedFit)) {
            ImGui::TableSetupColumn("##Label", ImGuiTableColumnFlags_WidthFixed, 95.0f);
            ImGui::TableSetupColumn("##Widget", ImGuiTableColumnFlags_WidthFixed, 145.0f);
            ImGui::TableSetupColumn("##map", ImGuiTableColumnFlags_WidthStretch);

            if (resources->materialMap.count(name)) {
                Material* material = resources->materialMap[name];
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text(material->name.c_str());
                static char buf1[32] = "";
                ImGuiInputTextFlags flags;
                flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ElideLeft;
                ImGui::SameLine();

                ImGui::TableSetColumnIndex(1);
                if (ImGui::InputText("##defaultMatFilename", buf1, IM_ARRAYSIZE(buf1), flags)) {
                    std::filesystem::path path(editor->fileClicked);
                    if (std::filesystem::is_regular_file(path)) {
                        std::filesystem::path newPath = path;
                        newPath.replace_filename(buf1 + extension);
                        std::filesystem::rename(path, newPath);
                        resources->materialMap.erase(name);
                        material->name = buf1 + extension;
                        resources->materialMap[material->name] = material;
                        editor->fileClicked = newPath.string();
                        saveScene(scene, resources);  // have to write the entire scene because mesh renderers get their material references by filename, and writing just the mesh renderers would be a disaster. Would need a stable handle to the material file, such as a guid, to skip this.
                    }
                }

                buildMaterialInspector(resources, material);
            }
            ImGui::EndTable();
        }
    }
}

void buildInspector(Scene* scene, Resources* resources, RenderState* renderer, EditorState* editor) {
    ImGui::Begin("Inspector");

    switch (editor->inspectorState) {
        case Empty:
            break;
        case SceneEntity:
            buildSceneEntityInspector(scene, renderer, resources, editor);
            break;
        case Prefab:
            buildPrefabInspector(scene);
            break;
        case Resource:
            buildResourceInspector(scene, resources, editor);
            break;
    }

    ImGui::End();
}