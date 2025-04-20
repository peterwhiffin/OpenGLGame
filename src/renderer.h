#pragma once
#include "component.h"

Entity* createEntityFromModel(Model* model, ModelNode* node, std::vector<MeshRenderer*>* renderers, Entity* parentEntity, bool first, unsigned int nextEntityID);
void drawPickingScene(std::vector<MeshRenderer*>& renderers, Camera& camera, unsigned int pickingShader);
void drawScene(std::vector<MeshRenderer*>& renderers, Camera& camera, Entity* nodeClicked, bool enableDirLight, DirectionalLight* sun);
void createPickingFBO(unsigned int* fbo, unsigned int* rbo, unsigned int* texture, glm::ivec2 screenSize);
void setFlags();