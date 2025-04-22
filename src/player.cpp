#include "player.h"
#include "transform.h"

void updatePlayer(GLFWwindow* window, InputActions* input, Player* player) {
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

Player* createPlayer(unsigned int* nextEntityID, float aspectRatio, std::vector<Entity>* entities, std::vector<BoxCollider>* colliders, std::vector<RigidBody>* rigidbodies, std::vector<Camera>* cameras) {
    Entity* playerEntity = getNewEntity(entities, nextEntityID);
    Entity* cameraTarget = getNewEntity(entities, nextEntityID);
    Entity* cameraEntity = getNewEntity(entities, nextEntityID);
    Player* player = new Player();
    Camera* camera = addCamera(cameraEntity, glm::radians(68.0f), aspectRatio, 0.01f, 800.0f, cameras);

    playerEntity->name = "Player";
    cameraTarget->name = "Camera Target";
    cameraEntity->name = "Camera";

    player->entity = playerEntity;
    player->collider = addBoxCollider(playerEntity, glm::vec3(0.0f), glm::vec3(0.25f, 0.9f, 0.25), colliders);
    player->rigidbody = addRigidBody(playerEntity, 20.0f, 0.0f, 0.0f, rigidbodies);
    player->cameraController = new CameraController();
    player->cameraController->camera = camera;
    player->cameraController->entity = playerEntity;
    player->cameraController->cameraTarget = &cameraTarget->transform;
    player->cameraController->transform = &playerEntity->transform;

    setParent(&cameraTarget->transform, &playerEntity->transform);
    setPosition(&playerEntity->transform, glm::vec3(0.0f, 3.0f, 0.0f));
    setLocalPosition(&cameraTarget->transform, glm::vec3(0.0f, 0.7f, 0.0f));

    return player;
}