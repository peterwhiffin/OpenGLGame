#pragma once
#include <glfw/glfw3.h>
#include "component.h"
#include "input.h"
void updatePlayer(GLFWwindow* window, InputActions* input, Player* player, std::vector<BoxCollider*>& colliders);
