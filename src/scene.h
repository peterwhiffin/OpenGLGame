#pragma once
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <unordered_map>
#include <unordered_set>
#include <string>

#include "renderer.h"
#include "input.h"
#include "physics.h"
#include "transform.h"
#include "animation.h"
#include "player.h"
#include "camera.h"
#include "shader.h"
#include "ecs.h"
#include "loader.h"
#include "editor.h"
#include "inspector.h"

struct Scene {
    bool menuOpen = false;
    bool menuCanOpen = true;

    double physicsAccum = 0.0f;
    double currentFrame = 0.0f;
    double lastFrame = 0.0f;
    double deltaTime;

    float gravity = -18.81f;

#ifdef PETES_EDITOR
    uint32_t pickedEntity = INVALID_ID;
#endif

    uint32_t nextEntityID = 1;
    vec3 wrenchOffset = vec3(0.0f, -0.42f, 0.37f);

    std::string name = "default";
    std::string scenePath = "";

    InputActions* input;
    DirectionalLight sun;

    PhysicsScene physicsScene;
    EntityGroup entities;
};

void clearScene(Scene* scene);