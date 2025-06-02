#include "scene.h"

void clearScene(Scene* scene) {
    EntityGroup* entities = &scene->entities;
    while (entities->entities.size() > 0) {
        destroyEntity(entities, entities->entities[entities->entities.size() - 1].entityID);
    }

    /*     for (Camera* cam : scene->cameras) {
            // delete cam;
            free(cam);
        }
     */
    /* for (int i = 0; i < scene->cameras.size(); i++){
        Camera newCam;
        scene->cameras[i] = newCam;
    }

        scene->cameras.clear(); */

    // delete scene->player->cameraController;
    // delete scene->player;

    // scene->player = nullptr;

    /* Player newPlayer;
    CameraController newCamController;
    scene->player = newPlayer;
    scene->player.cameraController = newCamController; */
}