#pragma once
#include "component.h"

void setFlags();
void drawPickingScene(Scene* scene, unsigned int pickingFBO, unsigned int pickingShader);
void drawScene(Scene* scene, uint32_t nodeClicked);
void createPickingFBO(Scene* scene, unsigned int* fbo, unsigned int* rbo, unsigned int* texture);