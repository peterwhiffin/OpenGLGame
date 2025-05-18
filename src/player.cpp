#include "player.h"
#include "transform.h"
#include "animation.h"
#include "physics.h"

void spawnTrashCan(Scene* scene, Player* player) {
    uint32_t trashcanID = createEntityFromModel(scene, scene->trashcanModel->rootNode, INVALID_ID, false, INVALID_ID, true, true);
    // RigidBody* rb = getRigidbody(scene, trashcanID);
    Transform* transform = getTransform(scene, trashcanID);
    // getBoxCollider(scene, transform->childEntityIds[0])->isActive = false;
    JPH::CylinderShapeSettings floor_shape_settings(scene->trashcanModel->rootNode->mesh->extent.GetY(), scene->trashcanModel->rootNode->mesh->extent.GetX());
    // floor_shape_settings.SetEmbedded();  // A ref counted object on the stack (base class RefTarget) should be marked as such to prevent it from being freed when its reference count goes to 0.

    JPH::ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
    JPH::ShapeRefC floor_shape = floor_shape_result.Get();  // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()
    JPH::ObjectLayer layer = Layers::MOVING;
    JPH::EActivation shouldActivate = JPH::EActivation::Activate;
    JPH::EMotionType motionType = JPH::EMotionType::Dynamic;
    JPH::BodyCreationSettings floor_settings(floor_shape, JPH::RVec3(0.0_r, 0.0_r, 0.0_r), quat::sIdentity(), motionType, layer);
    JPH::Body* floor = scene->bodyInterface->CreateBody(floor_settings);
    scene->bodyInterface->AddBody(floor->GetID(), shouldActivate);

    RigidBody* rb = addRigidbody(scene, trashcanID);
    rb->joltBody = floor->GetID();
    // rb->mass = 10.0f;
    // rb->linearDrag = 3.0f;
    // rb->friction = 10.0f;
    vec3 camForward = forward(scene, player->cameraController->camera->entityID);
    // rb->linearVelocity = camForward * 20.0f;
    // setPosition(scene, trashcanID, getPosition(scene, player->cameraController->camera->entityID) + camForward);
    scene->bodyInterface->SetPosition(rb->joltBody, getPosition(scene, player->cameraController->camera->entityID) + camForward, JPH::EActivation::Activate);
    scene->bodyInterface->SetLinearVelocity(rb->joltBody, camForward * 20);
    rb->lastPosition = getPosition(scene, trashcanID);
    rb->lastRotation = getRotation(scene, trashcanID);
}

void updatePlayer(Scene* scene) {
    GLFWwindow* window = scene->window;
    Player* player = scene->player;
    InputActions* input = &scene->input;
    Transform* transform = getTransform(scene, player->entityID);

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
        if (input->fire) {
            if (scene->canPick) {
                scene->isPicking = true;
                scene->canPick = false;
            }
        } else {
            scene->isPicking = false;
            scene->canPick = true;
        }

        return;
    } else {
        // scene->nodeClicked = INVALID_ID;
    }

    if (input->spawn) {
        if (player->canSpawnCan) {
            player->canSpawnCan = false;
            spawnTrashCan(scene, player);
        }
    } else {
        player->canSpawnCan = true;
    }

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

    vec3 cameraTargetRotation = vec3(JPH::DegreesToRadians(player->cameraController->pitch), 0.0f, 0.0f);
    vec3 playerRotation = vec3(0.0f, JPH::DegreesToRadians(player->cameraController->yaw), 0.0f);

    setLocalRotation(scene, player->cameraController->cameraTargetEntityID, quat::sEulerAngles(cameraTargetRotation));
    setRotation(scene, player->entityID, quat::sEulerAngles(playerRotation));

    vec3 moveDir = vec3(0.0f, 0.0f, 0.0f);
    vec3 forwardDir = forward(scene, player->entityID);
    vec3 rightDir = right(scene, player->entityID);
    moveDir += input->movement.y * forwardDir + input->movement.x * rightDir;
    vec3 finalMove = moveDir * player->moveSpeed;

    if (input->altFire) {
        playAnimation(getAnimator(scene, player->armsID), "FlipOff");
    } else if (input->movement.y == 0 && input->movement.x == 0) {
        playAnimation(getAnimator(scene, player->armsID), "WrenchIdle");
    } else {
        playAnimation(getAnimator(scene, player->armsID), "WrenchMove");
    }

    RigidBody* rb = getRigidbody(scene, player->entityID);

    if (rb == nullptr) {
        return;
    }

    finalMove.SetY(scene->bodyInterface->GetLinearVelocity(rb->joltBody).GetY());

    if (input->jump) {
        if (player->isGrounded && player->canJump) {
            finalMove.SetY(player->jumpHeight);
            player->canJump = false;
        }
    } else {
        player->canJump = true;
    }

    player->isGrounded = true;
    scene->bodyInterface->SetLinearVelocity(rb->joltBody, finalMove);
    // scene->bodyInterface->SetRotation(rb->joltBody, quat::sEulerAngles(playerRotation), JPH::EActivation::Activate);
}

Player* buildPlayer(Scene* scene) {
    uint32_t playerEntityID = getNewEntity(scene, "Player")->entityID;
    uint32_t cameraTargetEntityID = getNewEntity(scene, "CameraTarget")->entityID;
    uint32_t cameraEntityID = getNewEntity(scene, "Camera")->entityID;

    Camera* camera = addCamera(scene, cameraEntityID, 68.0f, (float)scene->windowData.width / scene->windowData.height, 0.01f, 800.0f);
    Player* player = new Player();
    player->entityID = playerEntityID;

    RigidBody* rb = addRigidbody(scene, playerEntityID);
    rb->rotationLocked = true;
    player->cameraController = new CameraController();

    setPosition(scene, playerEntityID, vec3(0.0f, 5.0f, 0.0f));

    // JPH::CharacterVirtualSettings* characterSettings = new JPH::CharacterVirtualSettings();
    // characterSettings->mSupportingVolume = JPH::Plane(vec3::sAxisY(), .25);

    JPH::BoxShapeSettings floor_shape_settings(vec3(0.25f, 0.9f, 0.25f));
    floor_shape_settings.SetEmbedded();  // A ref counted object on the stack (base class RefTarget) should be marked as such to prevent it from being freed when its reference count goes to 0.
    JPH::ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
    JPH::ShapeRefC floor_shape = floor_shape_result.Get();
    JPH::ObjectLayer layer = Layers::MOVING;
    JPH::EActivation shouldActivate = JPH::EActivation::Activate;
    JPH::BodyCreationSettings floor_settings(floor_shape, getPosition(scene, playerEntityID), getRotation(scene, playerEntityID), JPH::EMotionType::Dynamic, layer);
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
    return player;
}