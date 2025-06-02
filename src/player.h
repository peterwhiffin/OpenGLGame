#pragma once
#include "forward.h"

struct CameraController {
    uint32_t entityID;
    uint32_t cameraTargetEntityID;
    uint32_t cameraEntityID;
    Camera* camera;
    float pitch = 0;
    float yaw = 0;
    float sensitivity = .3;
    float moveSpeed = 10;
};

struct Player {
    uint32_t entityID;
    CameraController cameraController;
    uint32_t armsID;
    bool isGrounded = false;
    bool canJump = true;
    bool canSpawnCan = true;
    float jumpHeight = 10.0f;
    float moveSpeed = 10.0f;
    float groundCheckDistance = 0.2f;
};

void updatePlayer(Scene* scene, Resources* resources, RenderState* renderer);
Player* buildPlayer(Scene* scene);