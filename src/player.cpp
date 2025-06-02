#include <glad/glad.h>
#include <glfw/glfw3.h>

#include "player.h"
#include "scene.h"
#include "loader.h"
#include "physics.h"
#include "transform.h"
#include "renderer.h"
#include "input.h"
#include "camera.h"
#include "animation.h"
#include "ecs.h"

static void spawnTrashCan(Scene* scene, Resources* resources, Player* player) {
    EntityGroup* entities = &scene->entities;
    JPH::BodyInterface* bodyInterface = scene->physicsScene.bodyInterface;
    Model* trashcanModel = resources->modelMap["trashcan.gltf"];
    uint32_t trashcanID = createEntityFromModel(entities, &scene->physicsScene, trashcanModel->rootNode, INVALID_ID, false, INVALID_ID, true, true);
    Transform* transform = getTransform(entities, trashcanID);
    JPH::CylinderShapeSettings floor_shape_settings(trashcanModel->rootNode->mesh->extent.GetY(), trashcanModel->rootNode->mesh->extent.GetX());
    JPH::ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
    JPH::ShapeRefC floor_shape = floor_shape_result.Get();  // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()
    JPH::ObjectLayer layer = Layers::MOVING;
    JPH::EActivation shouldActivate = JPH::EActivation::Activate;
    JPH::EMotionType motionType = JPH::EMotionType::Dynamic;
    JPH::BodyCreationSettings floor_settings(floor_shape, JPH::RVec3(0.0_r, 0.0_r, 0.0_r), quat::sIdentity(), motionType, layer);
    JPH::Body* floor = bodyInterface->CreateBody(floor_settings);
    bodyInterface->AddBody(floor->GetID(), shouldActivate);

    RigidBody* rb = addRigidbody(entities, trashcanID);
    rb->joltBody = floor->GetID();
    vec3 camForward = transformForward(entities, player->cameraController.camera->entityID);
    bodyInterface->SetPosition(rb->joltBody, getPosition(entities, player->cameraController.camera->entityID) + camForward, JPH::EActivation::Activate);
    bodyInterface->SetLinearVelocity(rb->joltBody, camForward * 20);
    rb->lastPosition = getPosition(entities, trashcanID);
    rb->lastRotation = getRotation(entities, trashcanID);
    entities->movingRigidbodies.insert(rb->entityID);
}

void updatePlayer(Scene* scene, Resources* resources, RenderState* renderer) {
    GLFWwindow* window = renderer->window;
    EntityGroup* entities = &scene->entities;
    JPH::BodyInterface* bodyInterface = scene->physicsScene.bodyInterface;
    Player* player = &scene->entities.players[0];
    InputActions* input = scene->input;
    Transform* transform = getTransform(entities, player->entityID);

    if (input->menu) {
        if (scene->menuCanOpen) {
            scene->menuOpen = !scene->menuOpen;
            scene->menuCanOpen = false;

            glfwSetInputMode(window, GLFW_CURSOR, scene->menuOpen ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
        }
    } else {
        scene->menuCanOpen = true;
    }

    if (scene->menuOpen) {
        return;
    }

    if (input->spawn) {
        if (player->canSpawnCan) {
            player->canSpawnCan = false;
            spawnTrashCan(scene, resources, player);
        }
    } else {
        player->canSpawnCan = true;
    }

    float xOffset = input->lookX * player->cameraController.sensitivity;
    float yOffset = input->lookY * player->cameraController.sensitivity;
    float pitch = player->cameraController.pitch;
    float yaw = player->cameraController.yaw;

    yaw -= xOffset;
    pitch -= yOffset;

    if (pitch > 89.0f) {
        pitch = 89.0f;
    }

    if (pitch < -89.0f) {
        pitch = -89.0f;
    }

    player->cameraController.pitch = pitch;
    player->cameraController.yaw = yaw;

    vec3 cameraTargetRotation = vec3(JPH::DegreesToRadians(player->cameraController.pitch), 0.0f, 0.0f);
    vec3 playerRotation = vec3(0.0f, JPH::DegreesToRadians(player->cameraController.yaw), 0.0f);

    setRotation(entities, player->entityID, quat::sEulerAngles(playerRotation));
    setLocalRotation(entities, player->cameraController.cameraTargetEntityID, quat::sEulerAngles(cameraTargetRotation));

    vec3 moveDir = vec3(0.0f, 0.0f, 0.0f);
    vec3 forwardDir = transformForward(entities, player->entityID);
    vec3 rightDir = transformRight(entities, player->entityID);
    moveDir += input->movement.y * forwardDir + input->movement.x * rightDir;
    vec3 finalMove = moveDir * player->moveSpeed;

    if (input->altFire) {
        playAnimation(getAnimator(entities, player->armsID), "FlipOff");
    } else if (input->movement.y == 0 && input->movement.x == 0) {
        playAnimation(getAnimator(entities, player->armsID), "WrenchIdle");
    } else {
        playAnimation(getAnimator(entities, player->armsID), "WrenchMove");
    }

    RigidBody* rb = getRigidbody(entities, player->entityID);

    if (rb == nullptr) {
        return;
    }

    finalMove.SetY(bodyInterface->GetLinearVelocity(rb->joltBody).GetY());

    if (input->jump) {
        if (player->isGrounded && player->canJump) {
            finalMove.SetY(player->jumpHeight);
            player->canJump = false;
        }
    } else {
        player->canJump = true;
    }

    player->isGrounded = true;
    bodyInterface->SetLinearVelocity(rb->joltBody, finalMove);
}

Player* buildPlayer(Scene* scene) {
    return nullptr;
    /* uint32_t playerEntityID = getNewEntity(scene, "Player")->entityID;
    uint32_t cameraTargetEntityID = getNewEntity(scene, "CameraTarget")->entityID;
    uint32_t cameraEntityID = getNewEntity(scene, "Camera")->entityID;

    Camera* camera = addCamera(scene, cameraEntityID, 68.0f, 0.01f, 800.0f);
    Player* player = new Player();
    player->entityID = playerEntityID;

    RigidBody* rb = addRigidbody(scene, playerEntityID);
    rb->rotationLocked = true;
    player->cameraController = new CameraController();

    setPosition(scene, playerEntityID, vec3(0.0f, 5.0f, 0.0f));

    JPH::BoxShapeSettings floor_shape_settings(vec3(0.25f, 0.9f, 0.25f));
    floor_shape_settings.SetEmbedded();  // A ref counted object on the stack (base class RefTarget) should be marked as such to prevent it from being freed when its reference count goes to 0.
    JPH::ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
    JPH::ShapeRefC floor_shape = floor_shape_result.Get();
    JPH::ObjectLayer layer = Layers::MOVING;
    JPH::EActivation shouldActivate = JPH::EActivation::Activate;
    JPH::BodyCreationSettings floor_settings(floor_shape, getPosition(scene, playerEntityID), getRotation(scene, playerEntityID), JPH::EMotionType::Kinematic, layer);
    floor_settings.mAllowedDOFs = JPH::EAllowedDOFs::TranslationX | JPH::EAllowedDOFs::TranslationY | JPH::EAllowedDOFs::TranslationZ;
    JPH::Body* floor = scene->bodyInterface->CreateBody(floor_settings);

    rb->joltBody = floor->GetID();
    rb->lastPosition = getPosition(scene, playerEntityID);
    scene->bodyInterface->AddBody(floor->GetID(), shouldActivate);
    scene->bodyInterface->SetLinearVelocity(floor->GetID(), vec3(0.0f, 0.0f, 0.0f));
    player->cameraController->camera = camera;
    player->cameraController->entityID = playerEntityID;
    player->cameraController->cameraTargetEntityID = cameraTargetEntityID;
    player->cameraController->cameraEntityID = cameraEntityID;

    setParent(scene, cameraTargetEntityID, playerEntityID);
    setLocalPosition(scene, cameraTargetEntityID, vec3(0.0f, 0.7f, 0.0f));

    scene->player = player;
    scene->movingRigidbodies.insert(rb->entityID);
    return player; */
}