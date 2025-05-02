#pragma once
#include "component.h"
#include "player.h"
#include "utils/imgui.h"
#include "utils/imgui_impl_glfw.h"
#include "utils/imgui_impl_opengl3.h"

void checkPicker(Scene* scene, glm::dvec2 pickPosition);
void buildImGui(Scene* scene, ImGuiTreeNodeFlags node_flags, Player* player);
void drawDebug(Scene* scene, ImGuiTreeNodeFlags nodeFlags, Player* player);