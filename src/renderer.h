#pragma once
#include "component.h"

void setFlags();
void drawPickingScene(std::vector<MeshRenderer>& renderers, Camera& camera, unsigned int pickingFBO, unsigned int pickingShader, WindowData* WindowData);
void drawScene(std::vector<MeshRenderer>& renderers, Camera& camera, Entity* nodeClicked, bool enableDirLight, DirectionalLight* sun, WindowData* windowData);
void createPickingFBO(unsigned int* fbo, unsigned int* rbo, unsigned int* texture, WindowData* windowData);