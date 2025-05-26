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

void buildFloatRow(std::string label, float* value, float speed = 0.01f, float min = 0.0f, float max = 1.0f) {
    std::string hideLabel = "##" + label;
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text(label.c_str());
    ImGui::TableSetColumnIndex(1);
    ImGui::DragFloat(hideLabel.c_str(), value, speed, min, max);
}

void buildMaterialInspector(Scene* scene, Material* material) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Material");
    ImGui::TableSetColumnIndex(1);
    if (ImGui::BeginCombo("##currentMaterial", material->name.c_str())) {
        const bool isSelected = false;

        for (auto& pair : scene->materialMap) {
            if (ImGui::Selectable(pair.first.c_str(), isSelected)) {
                material = pair.second;
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
}

void buildMeshRendererInspector(Scene* scene, MeshRenderer* renderer) {
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

                for (auto& pair : scene->meshMap) {
                    if (ImGui::Selectable(pair.first.c_str(), isSelected)) {
                        renderer->mesh = pair.second;
                    }
                }

                ImGui::EndCombo();
            }

            if (renderer->mesh != nullptr) {
                buildMaterialInspector(scene, renderer->mesh->subMeshes[0].material);
            }

            ImGui::EndTable();
        }
    }
}

void buildAnimatorInspector(Scene* scene, Animator* animator) {
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

void buildRigidbodyInspector(Scene* scene, RigidBody* rigidbody) {
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

void buildPointLightInspector(Scene* scene, PointLight* light) {
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

void buildSpotLightInspector(Scene* scene, SpotLight* spotLight) {
    if (ImGui::CollapsingHeader("Spot Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable("Spot Light Table", 2, ImGuiTableFlags_SizingFixedSame)) {
            ImGui::TableSetupColumn("##Label", ImGuiTableColumnFlags_None, 0.0f, 200.0f);
            ImGui::TableSetupColumn("##Widget", ImGuiTableColumnFlags_WidthStretch);

            buildFloatRow("Brightness", &spotLight->brightness, 0.01f, 0.0f, 1000.0f);

            /* ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Brightness");
            ImGui::TableSetColumnIndex(1);
            ImGui::DragFloat("##brightness", &spotLight->brightness, 0.01f, 0.0f, 1000.0f); */

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

void buildTextureInspector(Scene* scene, std::string path) {
}

void buildAddComponentCombo(Scene* scene) {
    if (ImGui::BeginCombo("##Add Component", "Add Component")) {
        const bool isSelected = false;
        uint32_t entityID = scene->nodeClicked;
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

void buildSceneEntityInspector(Scene* scene) {
    uint32_t entityID = scene->nodeClicked;
    Transform* transform = getTransform(scene, entityID);
    MeshRenderer* renderer = getMeshRenderer(scene, entityID);
    Animator* animator = getAnimator(scene, entityID);
    RigidBody* rigidbody = getRigidbody(scene, entityID);
    SpotLight* spotLight = getSpotLight(scene, entityID);
    PointLight* pointLight = getPointLight(scene, entityID);

    buildTransformInspector(scene, transform);

    if (animator != nullptr) {
        buildAnimatorInspector(scene, animator);
    }

    if (pointLight != nullptr) {
        buildPointLightInspector(scene, pointLight);
    }

    if (spotLight != nullptr) {
        buildSpotLightInspector(scene, spotLight);
    }

    if (rigidbody != nullptr) {
        buildRigidbodyInspector(scene, rigidbody);
    }

    if (renderer != nullptr) {
        buildMeshRendererInspector(scene, renderer);
    }

    buildAddComponentCombo(scene);
}

void buildPrefabInspector(Scene* scene) {
}

void buildResourceInspector(Scene* scene) {
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

void buildInspector(Scene* scene) {
    ImGui::Begin("Inspector");

    switch (scene->inspectorState) {
        case Empty:
            break;
        case SceneEntity:
            buildSceneEntityInspector(scene);
            break;
        case Prefab:
            buildPrefabInspector(scene);
            break;
        case Resource:
            buildResourceInspector(scene);
            break;
    }

    ImGui::End();
}