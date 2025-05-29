#include "scene.h"

void clearScene(Scene* scene) {
    while (scene->entities.size() > 0) {
        destroyEntity(scene, scene->entities[scene->entities.size() - 1].entityID);
    }

    for (Camera* cam : scene->cameras) {
        delete cam;
    }

    scene->cameras.clear();

    delete scene->player->cameraController;
    delete scene->player;
    scene->player = nullptr;
}