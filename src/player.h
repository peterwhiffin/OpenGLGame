#pragma once
#include "component.h"
#include "input.h"
void updatePlayer(GLFWwindow* window, InputActions* input, Player* player, std::vector<BoxCollider*>& colliders);
