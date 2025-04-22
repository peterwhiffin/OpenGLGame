#pragma once
#include "component.h"
#include "input.h"
void updatePlayer(GLFWwindow* window, InputActions* input, Player* player);
Player* createPlayer(unsigned int* nextEntityID, float aspectRatio, std::vector<Entity>* entities, std::vector<BoxCollider>* colliders, std::vector<RigidBody>* rigidbodies, std::vector<Camera>* cameras);