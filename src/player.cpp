#include "player.h"
#include "transform.h"

void updatePlayer(Scene* scene, GLFWwindow* window, InputActions* input, Player* player) {
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

    setLocalRotation(scene, player->cameraController->cameraTargetEntityID, glm::quat(cameraTargetRotation));
    setRotation(scene, player->entityID, glm::quat(playerRotation));

    glm::vec3 moveDir = glm::vec3(0.0f);
    moveDir += input->movement.y * forward(scene, player->entityID) + input->movement.x * right(scene, player->entityID);
    glm::vec3 finalMove = moveDir * player->moveSpeed;

    size_t index = scene->rigidbodyIndices[player->entityID];
    RigidBody* rb = &scene->rigidbodies[index];

    finalMove.y = rb->linearVelocity.y;

    if (player->isGrounded) {
        if (input->jump) {
            finalMove.y = player->jumpHeight;
        }
    }

    player->isGrounded = true;
    rb->linearVelocity = finalMove;
}

Player* createPlayer(Scene* scene) {
    uint32_t playerEntityID = getNewEntity(scene, "Player")->id;
    uint32_t cameraTargetEntityID = getNewEntity(scene, "Camera Target")->id;
    uint32_t cameraEntityID = getNewEntity(scene, "Camera")->id;

    Camera* camera = addCamera(scene, cameraEntityID, 68.0f, (float)scene->windowData.width / scene->windowData.height, 0.01f, 800.0f);
    Player* player = new Player();
    player->entityID = playerEntityID;

    BoxCollider* collider = addBoxCollider(scene, playerEntityID);
    RigidBody* rb = addRigidbody(scene, playerEntityID);
    player->cameraController = new CameraController();

    collider->center = glm::vec3(0.0f);
    collider->extent = glm::vec3(0.25f, 0.9f, 0.25f);
    collider->isActive = false;
    rb->mass = 20.0f;
    rb->linearDrag = 0.0f;
    rb->friction = 0.0f;

    player->cameraController->camera = camera;
    player->cameraController->entityID = playerEntityID;
    player->cameraController->cameraTargetEntityID = cameraTargetEntityID;

    setParent(scene, cameraTargetEntityID, playerEntityID);
    setPosition(scene, playerEntityID, glm::vec3(0.0f, 3.0f, 0.0f));
    setLocalPosition(scene, cameraTargetEntityID, glm::vec3(0.0f, 0.7f, 0.0f));

    return player;
}