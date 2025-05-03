#pragma once
#include "component.h"

void setFlags();
void drawPickingScene(Scene* scene);
void drawScene(Scene* scene);
void createPickingFBO(Scene* scene);
void createFullScreenQuad(Scene* scene);
void drawFullScreenQuad(Scene* scene);
void createForwardBuffer(Scene* scene);
void createBlurBuffers(Scene* scene);
void drawBlurPass(Scene* scene);
void generateSSAOKernel(Scene* scene);
void createDepthPrePassBuffer(Scene* scene);
void drawDepthPrePass(Scene* scene);
void resizeBuffers(Scene* scene);
void createShadowMapDepthBuffers(Scene* scene);
void drawShadowMaps(Scene* scene);
void createSSAOBuffer(Scene* scene);
void drawSSAO(Scene* scene);
void createSSAOBuffers(Scene* scene);