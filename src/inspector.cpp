// #include <iostream>
#include <filesystem>
// #include <fstream>

// #include "Jolt/Math/Math.h"
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
#include "meshrenderer.h"

void buildTextRow(std::string label, std::string value) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text(label.c_str());
    ImGui::TableSetColumnIndex(1);
    ImGui::Text(value.c_str());
}

bool buildBoolRow(std::string label, bool* value) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text(label.c_str());
    ImGui::TableSetColumnIndex(1);
    return ImGui::Checkbox(("##" + label).c_str(), value);
}

bool buildFloatRow(std::string label, float* value, float speed = 0.01f, float min = 0.0f, float max = 0.0f) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text(label.c_str());
    ImGui::TableSetColumnIndex(1);
    return ImGui::DragFloat(("##" + label).c_str(), value, speed, min, max);
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

void buildTextureMapRow(Resources* resources, std::string label, Texture* tex, float* value, float max = 1.0f, bool color = false) {
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
        ImGui::ColorEdit4(("##" + label + "color").c_str(), value, ImGuiColorEditFlags_HDR);
    } else {
        ImGui::TableSetColumnIndex(2);
        ImGui::DragFloat(("##max" + label).c_str(), value, 0.001f, 0.0f, max);
    }
}

Material* buildMaterialInspector(Resources* resources, Material* material, bool forMesh) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("Material");
    ImGui::TableSetColumnIndex(1);
    if (forMesh) {
        if (ImGui::BeginCombo("##currentMaterial", material->name.c_str())) {
            const bool isSelected = false;

            for (auto& pair : resources->materialMap) {
                if (ImGui::Selectable(pair.first.c_str(), isSelected)) {
                    material = pair.second;
                }
            }

            ImGui::EndCombo();
        }
    }

    buildTextureMapRow(resources, "Albedo", material->textures[0], material->baseColor.mF32, 1.0f, true);
    buildTextureMapRow(resources, "Roughness", material->textures[1], &material->roughness);
    buildTextureMapRow(resources, "Metalness", material->textures[2], &material->metalness);
    buildTextureMapRow(resources, "AO", material->textures[3], &material->aoStrength);
    buildTextureMapRow(resources, "Normal", material->textures[4], &material->normalStrength, 0.0f);
    buildFloat2Row("Tiling", glm::value_ptr(material->textureTiling), 0.001f);
    return material;
}

void buildTransformInspector(Scene* scene, Transform* transform) {
    EntityGroup* entities = &scene->entities;
    uint32_t entityID = transform->entityID;
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable("Transform Table", 2, ImGuiTableFlags_SizingFixedSame)) {
            Transform* transform = getTransform(entities, entityID);
            vec3 position = transform->localPosition;
            vec3 worldPosition = transform->worldTransform.GetTranslation();
            vec3 rotation = getLocalRotation(entities, entityID).GetEulerAngles();
            vec3 degrees = vec3(JPH::RadiansToDegrees(rotation.GetX()), JPH::RadiansToDegrees(rotation.GetY()), JPH::RadiansToDegrees(rotation.GetZ()));
            vec3 scale = transform->localScale;
            ImGui::TableSetupColumn("##Label", ImGuiTableColumnFlags_None, 0.0f, 200.0f);
            ImGui::TableSetupColumn("##Widget", ImGuiTableColumnFlags_WidthStretch);

            buildFloat3Row("World Position", worldPosition.mF32, 0.1f);
            bool posChanged = buildFloat3Row("Position", position.mF32);
            bool rotChanged = buildFloat3Row("Rotation", degrees.mF32);
            bool scaleChanged = buildFloat3Row("Scale", scale.mF32);

            vec3 newPos;
            newPos.SetX(position.GetX());
            newPos.SetY(position.GetY());
            newPos.SetZ(position.GetZ());
            vec3 radians = vec3(JPH::DegreesToRadians(degrees.GetX()), JPH::DegreesToRadians(degrees.GetY()), JPH::DegreesToRadians(degrees.GetZ()));
            RigidBody* rb = getRigidbody(entities, entityID);
            if (rb != nullptr) {
                if (posChanged || rotChanged) {
                    scene->physicsScene.bodyInterface->SetPositionAndRotation(rb->joltBody, newPos, quat::sEulerAngles(radians), JPH::EActivation::Activate);
                    setLocalPosition(entities, entityID, position);
                    setLocalRotation(entities, entityID, quat::sEulerAngles(radians));
                }
                if (scaleChanged) {
                    setLocalScale(entities, entityID, scale);
                }
            } else {
                setLocalPosition(entities, entityID, position);
                setLocalRotation(entities, entityID, quat::sEulerAngles(radians));
                setLocalScale(entities, entityID, scale);
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
            removeMeshRenderer(&scene->entities, renderer->entityID);
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

            Entity* rootEntity = getEntity(&scene->entities, renderer->rootEntity);
            buildTextRow("Root Bone: ", rootEntity->name);

            if (renderer->mesh != nullptr) {
                Material* mat = renderer->mesh->subMeshes[0].material;
                mat = buildMaterialInspector(resources, mat, true);
                renderer->mesh->subMeshes[0].material = mat;
            }

            ImGui::EndTable();
        }
    }
}

void buildAnimatorInspector(Scene* scene, Animator* animator) {
    bool isOpen = ImGui::CollapsingHeader("Animator", ImGuiTreeNodeFlags_DefaultOpen);
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        ImGui::OpenPopup("AnimatorContextMenu");
    }
    if (ImGui::BeginPopup("AnimatorContextMenu")) {
        if (ImGui::MenuItem("Remove")) {
            removeAnimator(&scene->entities, animator->entityID);
        }

        ImGui::EndPopup();
    }

    if (isOpen) {
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
    EntityGroup* entities = &scene->entities;
    JPH::BodyInterface* bodyInterface = scene->physicsScene.bodyInterface;
    const JPH::Shape* shape = bodyInterface->GetShape(rigidbody->joltBody).GetPtr();
    const JPH::BoxShape* box;
    const JPH::SphereShape* sphere;
    const JPH::CapsuleShape* capsule;
    const JPH::CylinderShape* cylinder;
    JPH::EShapeSubType shapeType = shape->GetSubType();
    JPH::Color color(0, 255, 0);
    JPH::AABox localBox;

    float radius = 0.5f;
    float halfHeight = 0.5f;
    std::string shapeComboPreview = "NULL";
    std::vector<std::string> shapes;
    shapes.push_back("Box");
    shapes.push_back("Sphere");
    shapes.push_back("Capsule");
    shapes.push_back("Cylinder");

    bool isOpen = ImGui::CollapsingHeader("Rigidbody", ImGuiTreeNodeFlags_DefaultOpen);
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        ImGui::OpenPopup("RigidbodyContextMenu");
    }
    if (ImGui::BeginPopup("RigidbodyContextMenu")) {
        if (ImGui::MenuItem("Remove")) {
            removeRigidbody(entities, rigidbody->entityID);
        }

        ImGui::EndPopup();
    }

    if (isOpen) {
        if (ImGui::BeginTable("Rigidbody Table", 2, ImGuiTableFlags_SizingFixedSame)) {
            ImGui::TableSetupColumn("##Label", ImGuiTableColumnFlags_None, 0.0f, 200.0f);
            ImGui::TableSetupColumn("##Widget", ImGuiTableColumnFlags_WidthStretch);

            switch (shapeType) {
                case JPH::EShapeSubType::Box:
                    shapeComboPreview = "Box";
                    box = static_cast<const JPH::BoxShape*>(shape);
                    vec3 halfExtents = box->GetHalfExtent();
                    localBox = JPH::AABox(-halfExtents, halfExtents);
                    renderer->debugRenderer->DrawBox(bodyInterface->GetWorldTransform(rigidbody->joltBody), localBox, color, JPH::DebugRenderer::ECastShadow::Off, JPH::DebugRenderer::EDrawMode::Wireframe);
                    break;
                case JPH::EShapeSubType::Sphere:
                    shapeComboPreview = "Sphere";
                    sphere = static_cast<const JPH::SphereShape*>(shape);
                    radius = sphere->GetRadius();
                    renderer->debugRenderer->DrawSphere(bodyInterface->GetPosition(rigidbody->joltBody), radius, color, JPH::DebugRenderer::ECastShadow::Off, JPH::DebugRenderer::EDrawMode::Wireframe);
                    break;
                case JPH::EShapeSubType::Capsule:
                    shapeComboPreview = "Capsule";
                    capsule = static_cast<const JPH::CapsuleShape*>(shape);
                    halfHeight = capsule->GetHalfHeightOfCylinder();
                    radius = capsule->GetRadius();
                    renderer->debugRenderer->DrawCapsule(bodyInterface->GetWorldTransform(rigidbody->joltBody), halfHeight, radius, color, JPH::DebugRenderer::ECastShadow::Off, JPH::DebugRenderer::EDrawMode::Wireframe);
                    break;
                case JPH::EShapeSubType::Cylinder:
                    shapeComboPreview = "Cylinder";
                    cylinder = static_cast<const JPH::CylinderShape*>(shape);
                    halfHeight = cylinder->GetHalfHeight();
                    radius = cylinder->GetRadius();
                    renderer->debugRenderer->DrawCylinder(bodyInterface->GetWorldTransform(rigidbody->joltBody), halfHeight, radius, color, JPH::DebugRenderer::ECastShadow::Off, JPH::DebugRenderer::EDrawMode::Wireframe);
                    break;
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Shape");
            ImGui::TableSetColumnIndex(1);

            if (ImGui::BeginCombo("##BodyShape", shapeComboPreview.c_str())) {
                const bool isSelected = false;
                for (std::string& shape : shapes) {
                    if (shape != shapeComboPreview) {
                        if (ImGui::Selectable(shape.c_str(), isSelected)) {
                            if (shape == "Box") {
                                vec3 newExtent = vec3(0.5f, 0.5f, 0.5f);
                                JPH::BoxShapeSettings boxShapeSettings(newExtent);
                                JPH::ShapeSettings::ShapeResult boxResult = boxShapeSettings.Create();
                                JPH::ShapeRefC boxShape = boxResult.Get();
                                bodyInterface->SetShape(rigidbody->joltBody, boxShape, false, JPH::EActivation::DontActivate);
                                shapeType = JPH::EShapeSubType::Box;
                            } else if (shape == "Sphere") {
                                JPH::SphereShapeSettings sphereShapeSettings(0.5f);
                                JPH::ShapeSettings::ShapeResult sphereResult = sphereShapeSettings.Create();
                                JPH::ShapeRefC sphereShape = sphereResult.Get();
                                bodyInterface->SetShape(rigidbody->joltBody, sphereShape, false, JPH::EActivation::DontActivate);
                                shapeType = JPH::EShapeSubType::Sphere;
                            } else if (shape == "Capsule") {
                                JPH::CapsuleShapeSettings capsuleShapeSettings(0.5f, 0.5f);
                                JPH::ShapeSettings::ShapeResult capsuleResult = capsuleShapeSettings.Create();
                                JPH::ShapeRefC capsuleShape = capsuleResult.Get();
                                bodyInterface->SetShape(rigidbody->joltBody, capsuleShape, false, JPH::EActivation::DontActivate);
                                shapeType = JPH::EShapeSubType::Capsule;
                            } else if (shape == "Cylinder") {
                                JPH::CylinderShapeSettings cylinderShapeSettings(0.5f, 0.5f);
                                JPH::ShapeSettings::ShapeResult cylinderResult = cylinderShapeSettings.Create();
                                JPH::ShapeRefC cylinderShape = cylinderResult.Get();
                                bodyInterface->SetShape(rigidbody->joltBody, cylinderShape, false, JPH::EActivation::DontActivate);
                                shapeType = JPH::EShapeSubType::Cylinder;
                            }
                        }
                    }
                }
                ImGui::EndCombo();
            }

            // const JPH::Shape* shape = scene->bodyInterface->GetShape(rigidbody->joltBody).GetPtr();
            // JPH::EShapeSubType shapeType = shape->GetSubType();
            JPH::ObjectLayer objectLayer = bodyInterface->GetObjectLayer(rigidbody->joltBody);
            JPH::EMotionType motionType = bodyInterface->GetMotionType(rigidbody->joltBody);
            std::string motionTypeString = motionType == JPH::EMotionType::Dynamic ? "Dynamic" : "Static";
            const JPH::BoxShape* box = static_cast<const JPH::BoxShape*>(shape);
            vec3 halfExtents = box->GetHalfExtent();
            buildFloat3Row("Center: ", rigidbody->center.mF32);

            if (shapeType == JPH::EShapeSubType::Box) {
                if (buildFloat3Row("Half Extents: ", halfExtents.mF32, 0.01f, 0.1f)) {
                    vec3 newExtent = vec3(std::max(halfExtents.GetX(), 0.0f), std::max(halfExtents.GetY(), 0.0f), std::max(halfExtents.GetZ(), 0.0f));
                    JPH::BoxShapeSettings boxShapeSettings(newExtent);
                    JPH::ShapeSettings::ShapeResult boxResult = boxShapeSettings.Create();
                    JPH::ShapeRefC boxShape = boxResult.Get();
                    bodyInterface->SetShape(rigidbody->joltBody, boxShape, false, JPH::EActivation::DontActivate);
                }
            } else if (shapeType == JPH::EShapeSubType::Sphere) {
                if (buildFloatRow("Radius: ", &radius, 0.01f, 0.1f)) {
                    JPH::SphereShapeSettings sphereShapeSettings(radius);
                    JPH::ShapeSettings::ShapeResult sphereResult = sphereShapeSettings.Create();
                    JPH::ShapeRefC sphereShape = sphereResult.Get();
                    bodyInterface->SetShape(rigidbody->joltBody, sphereShape, false, JPH::EActivation::DontActivate);
                }
            } else if (shapeType == JPH::EShapeSubType::Capsule) {
                if (buildFloatRow("Half Height: ", &halfHeight, 0.01f, 0.1f) || buildFloatRow("Radius: ", &radius, 0.01f, 0.1f)) {
                    JPH::CapsuleShapeSettings capsuleShapeSettings(halfHeight, radius);
                    JPH::ShapeSettings::ShapeResult capsuleResult = capsuleShapeSettings.Create();
                    JPH::ShapeRefC capsuleShape = capsuleResult.Get();
                    bodyInterface->SetShape(rigidbody->joltBody, capsuleShape, false, JPH::EActivation::DontActivate);
                }
            } else if (shapeType == JPH::EShapeSubType::Cylinder) {
                if (buildFloatRow("Half Height: ", &halfHeight, 0.01f, 0.1f) || buildFloatRow("Radius: ", &radius, 0.01f, 0.1f)) {
                    JPH::CylinderShapeSettings cylinderShapeSettings(halfHeight, radius);
                    JPH::ShapeSettings::ShapeResult cylinderResult = cylinderShapeSettings.Create();
                    JPH::ShapeRefC cylinderShape = cylinderResult.Get();
                    bodyInterface->SetShape(rigidbody->joltBody, cylinderShape, false, JPH::EActivation::DontActivate);
                }
            }

            switch (motionType) {
                case JPH::EMotionType::Dynamic:
                    motionTypeString = "Dynamic";
                    break;
                case JPH::EMotionType::Kinematic:
                    motionTypeString = "Kinematic";
                    break;
                case JPH::EMotionType::Static:
                    motionTypeString = "Static";
                    break;
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Motion Type");
            ImGui::TableSetColumnIndex(1);

            if (ImGui::BeginCombo("##Motion Type", motionTypeString.c_str())) {
                const bool isSelected = false;

                if (motionTypeString != "Dynamic") {
                    if (ImGui::Selectable("Dynamic", isSelected)) {
                        bodyInterface->DeactivateBody(rigidbody->joltBody);
                        bodyInterface->SetMotionType(rigidbody->joltBody, JPH::EMotionType::Dynamic, JPH::EActivation::Activate);
                        bodyInterface->SetObjectLayer(rigidbody->joltBody, Layers::MOVING);
                        setRigidbodyMoving(entities, rigidbody->entityID);
                        bodyInterface->ActivateBody(rigidbody->joltBody);
                    }
                }

                if (motionTypeString != "Kinematic") {
                    if (ImGui::Selectable("Kinematic", isSelected)) {
                        bodyInterface->DeactivateBody(rigidbody->joltBody);
                        bodyInterface->SetMotionType(rigidbody->joltBody, JPH::EMotionType::Kinematic, JPH::EActivation::Activate);
                        bodyInterface->SetObjectLayer(rigidbody->joltBody, Layers::MOVING);
                        setRigidbodyMoving(entities, rigidbody->entityID);
                        bodyInterface->ActivateBody(rigidbody->joltBody);
                    }
                }

                if (motionTypeString != "Static") {
                    if (ImGui::Selectable("Static", isSelected)) {
                        bodyInterface->SetMotionType(rigidbody->joltBody, JPH::EMotionType::Static, JPH::EActivation::Activate);
                        bodyInterface->SetObjectLayer(rigidbody->joltBody, Layers::NON_MOVING);
                        setRigidbodyNonMoving(entities, rigidbody->entityID);
                    }
                }

                ImGui::EndCombo();
            }

            rigidbody->motionType = motionType;
            rigidbody->shape = shapeType;
            rigidbody->halfExtents = halfExtents;
            rigidbody->halfHeight = halfHeight;
            rigidbody->radius = radius;
            rigidbody->layer = objectLayer;
            ImGui::EndTable();
        }
    }
}

void buildPointLightInspector(Scene* scene, PointLight* light) {
    bool isOpen = ImGui::CollapsingHeader("Point Light", ImGuiTreeNodeFlags_DefaultOpen);

    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        ImGui::OpenPopup("PointLightContextMenu");
    }

    if (ImGui::BeginPopup("PointLightContextMenu")) {
        if (ImGui::MenuItem("Remove")) {
            removePointLight(&scene->entities, light->entityID);
        }

        ImGui::EndPopup();
    }

    if (isOpen) {
        if (ImGui::BeginTable("Point Light Table", 2, ImGuiTableFlags_SizingFixedSame)) {
            ImGui::TableSetupColumn("##Label", ImGuiTableColumnFlags_None, 0.0f, 200.0f);
            ImGui::TableSetupColumn("##Widget", ImGuiTableColumnFlags_WidthStretch);

            buildFloatRow("Brightness", &light->brightness);
            buildColor3Row("Color", light->color.mF32, ImGuiColorEditFlags_DisplayHSV | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_HDR);
            ImGui::EndTable();
        }
    }
}

void buildSpotLightInspector(Scene* scene, SpotLight* spotLight) {
    bool isOpen = ImGui::CollapsingHeader("Spot Light", ImGuiTreeNodeFlags_DefaultOpen);
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        ImGui::OpenPopup("SpotLightContextMenu");
    }

    if (ImGui::BeginPopup("SpotLightContextMenu")) {
        if (ImGui::MenuItem("Remove")) {
            removeSpotLight(&scene->entities, spotLight->entityID);
        }

        ImGui::EndPopup();
    }

    if (isOpen) {
        if (ImGui::BeginTable("Spot Light Table", 2, ImGuiTableFlags_SizingFixedSame)) {
            ImGui::TableSetupColumn("##Label", ImGuiTableColumnFlags_None, 0.0f, 200.0f);
            ImGui::TableSetupColumn("##Widget", ImGuiTableColumnFlags_WidthStretch);

            buildBoolRow("Enabled", &spotLight->isActive);
            buildFloatRow("Brightness", &spotLight->brightness);
            buildColor3Row("Color", spotLight->color.mF32, ImGuiColorEditFlags_DisplayHSV | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_HDR);
            buildFloatRow("Inner Angle", &spotLight->cutoff, 0.01f, 0.0f, spotLight->outerCutoff - 0.01f);
            buildFloatRow("Outer Angle", &spotLight->outerCutoff, 0.01f, spotLight->cutoff + 0.01f, 180.0f);
            buildFloatRow("Range", &spotLight->range);
            buildFloatRow("Light Radius UV", &spotLight->lightRadiusUV, 0.0001f, 0.0f, 180.0f);
            buildFloatRow("Blocker Search UV", &spotLight->blockerSearchUV, 0.0001f, 0.0f, 180.0f);
            if (buildBoolRow("Shadows", &spotLight->enableShadows)) {
                if (spotLight->enableShadows) {
                    createSpotLightShadowMap(spotLight);
                } else {
                    deleteSpotLightShadowMap(spotLight);
                }
            }

            if (spotLight->enableShadows && ImGui::CollapsingHeader("Shadow Map")) {
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
        EntityGroup* entities = &scene->entities;
        const bool isSelected = false;
        uint32_t entityID = editor->nodeClicked;
        if (getMeshRenderer(entities, entityID) == nullptr) {
            if (ImGui::Selectable("Mesh Renderer", isSelected)) {
                MeshRenderer* renderer = addMeshRenderer(entities, entityID);
                renderer->mesh = nullptr;
                uint32_t parentID = getTransform(entities, entityID)->parentEntityID;
                uint32_t childID = entityID;
                while (parentID != INVALID_ID) {
                    childID = parentID;
                    parentID = getTransform(entities, childID)->parentEntityID;
                }

                renderer->rootEntity = childID;
            }
        }

        if (getRigidbody(entities, entityID) == nullptr) {
            if (ImGui::Selectable("Rigidbody: Box", isSelected)) {
                RigidBody* rb = addRigidbody(entities, entityID);
                JPH::BodyInterface* bodyInterface = scene->physicsScene.bodyInterface;
                JPH::BoxShapeSettings boxSettings(vec3(0.5f, 0.5f, 0.5f));
                JPH::ShapeSettings::ShapeResult shapeResult = boxSettings.Create();
                JPH::ShapeRefC shape = shapeResult.Get();
                JPH::BodyCreationSettings bodySettings(shape, JPH::RVec3(0.0_r, 0.0_r, 0.0_r), quat::sIdentity(), JPH::EMotionType::Static, Layers::NON_MOVING);
                bodySettings.mAllowDynamicOrKinematic = true;
                JPH::Body* body = bodyInterface->CreateBody(bodySettings);
                bodyInterface->AddBody(body->GetID(), JPH::EActivation::DontActivate);
                rb->joltBody = body->GetID();
            }
        }

        if (getSpotLight(entities, entityID) == nullptr) {
            if (ImGui::Selectable("Spot Light", isSelected)) {
                SpotLight* light = addSpotLight(entities, entityID);
                light->brightness = 10.0f;
                light->color = vec3(1.0f, 1.0f, 1.0f);
                light->cutoff = 1.0f;
                light->outerCutoff = 15.0f;
                createSpotLightShadowMap(light);
            }
        }

        ImGui::EndCombo();
    }
}

void buildCameraInspector(Scene* scene, Camera* camera) {
    bool isOpen = ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen);

    if (isOpen) {
        if (ImGui::BeginTable("Camera Table", 2, ImGuiTableFlags_SizingFixedSame)) {
            ImGui::TableSetupColumn("##Label", ImGuiTableColumnFlags_None, 0.0f, 200.0f);
            ImGui::TableSetupColumn("##Widget", ImGuiTableColumnFlags_WidthStretch);

            buildBoolRow("Perspective: ", &camera->isPerspective);

            if (buildFloatRow("FOV: ", &camera->fov, 0.01f, 0.1f, 180.0f)) {
                camera->fovRadians = JPH::DegreesToRadians(camera->fov);
            }

            buildFloatRow("Near Plane: ", &camera->nearPlane);
            buildFloatRow("Far Plane: ", &camera->farPlane);
            ImGui::EndTable();
        }
    }
}

void buildSceneEntityInspector(Scene* scene, RenderState* renderer, Resources* resources, EditorState* editor) {
    EntityGroup* entities = &scene->entities;
    uint32_t entityID = editor->nodeClicked;
    Transform* transform = getTransform(entities, entityID);
    MeshRenderer* meshRenderer = getMeshRenderer(entities, entityID);
    Animator* animator = getAnimator(entities, entityID);
    RigidBody* rigidbody = getRigidbody(entities, entityID);
    SpotLight* spotLight = getSpotLight(entities, entityID);
    PointLight* pointLight = getPointLight(entities, entityID);
    Camera* camera = getCamera(entities, entityID);

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
        buildRigidbodyInspector(scene, renderer, editor, rigidbody);
    }

    if (meshRenderer != nullptr) {
        buildMeshRendererInspector(scene, resources, meshRenderer);
    }

    if (camera != nullptr) {
        buildCameraInspector(scene, camera);
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

                buildMaterialInspector(resources, material, false);
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
