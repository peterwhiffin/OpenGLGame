#pragma once
#include "component.h"
#include "input.h"

void updatePlayer(Scene* scene, GLFWwindow* window, InputActions* input, Player* player);
Player* createPlayer(Scene* scene);