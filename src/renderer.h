#pragma once
#include "component.h"

void setFlags();
void drawPickingScene(Scene* scene, unsigned int pickingFBO, unsigned int pickingShader);
void drawScene(Scene* scene, uint32_t nodeClicked);
void createPickingFBO(Scene* scene, unsigned int* fbo, unsigned int* rbo, unsigned int* texture);
void createFullScreenQuad(Scene* scene);
void drawFullScreenQuad(Scene* scene);
void createForwardBuffer(Scene* scene);
void createBlurBuffers(Scene* scene);
void drawBlurPass(Scene* scene);
void generateSSAOKernel(Scene* scene);
void createDepthPrePassBuffer(Scene* scene);
void drawDepthPrePass(Scene* scene);