#pragma once
#include "component.h"
#include "input.h"

struct Player {
    uint32_t entityID;
    CameraController* cameraController;
    bool isGrounded = false;
    bool canJump = true;
    bool canSpawnCan = true;
    float jumpHeight = 10.0f;
    float moveSpeed = 10.0f;
    float groundCheckDistance = 0.2f;
};

void updatePlayer(Scene* scene, GLFWwindow* window, InputActions* input, Player* player);
Player* createPlayer(Scene* scene);