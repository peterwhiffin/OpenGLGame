#include "player.h"
#include "transform.h"

void updatePlayer(GLFWwindow* window, InputActions* input, Player* player, std::vector<BoxCollider*>& colliders) {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    float xOffset = input->lookX * player->cameraController->sensitivity;
    float yOffset = input->lookY * player->cameraController->sensitivity;
    float pitch = player->cameraController->pitch;
    float yaw = player->cameraController->yaw;

    yaw -= xOffset;
    pitch -= yOffset;

    if (pitch > 89.0f) {
        pitch = 89.0f;
    }

    if (pitch < -89.0f) {
        pitch = -89.0f;
    }

    player->cameraController->pitch = pitch;
    player->cameraController->yaw = yaw;

    float upTest = 0.0f;
    if (input->jump) {
        upTest = 1.0f;
    }

    glm::vec3 cameraTargetRotation = glm::vec3(glm::radians(player->cameraController->pitch), 0.0f, 0.0f);
    glm::vec3 playerRotation = glm::vec3(0.0f, glm::radians(player->cameraController->yaw), 0.0f);

    setLocalRotation(player->cameraController->cameraTarget, glm::quat(cameraTargetRotation));
    setRotation(&player->entity->transform, glm::quat(playerRotation));

    glm::vec3 moveDir = glm::vec3(0.0f);
    moveDir += input->movement.y * forward(&player->entity->transform) + input->movement.x * right(&player->entity->transform);
    glm::vec3 finalMove = moveDir * player->moveSpeed;

    finalMove.y = player->rigidbody->linearVelocity.y;
    if (player->isGrounded) {
        if (input->jump) {
            finalMove.y = player->jumpHeight;
        }
    }

    player->isGrounded = true;
    player->rigidbody->linearVelocity = finalMove;
}