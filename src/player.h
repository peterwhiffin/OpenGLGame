#pragma once
#include "component.h"
#include "input.h"
Player* createPlayer(Scene* scene);
void updatePlayer(Scene* scene, GLFWwindow* window, InputActions* input, Player* player);