#pragma once
#include "component.h"
#include "player.h"
#include "utils/imgui.h"
#include "utils/imgui_impl_glfw.h"
#include "utils/imgui_impl_opengl3.h"

void checkPicker(Scene* scene, glm::dvec2 pickPosition, uint32_t nodeClicked);
void buildImGui(Scene* scene, ImGuiTreeNodeFlags node_flags, uint32_t nodeClicked, Player* player);
void drawDebug(Scene* scene, ImGuiTreeNodeFlags nodeFlags, uint32_t nodeClicked, Player* player);