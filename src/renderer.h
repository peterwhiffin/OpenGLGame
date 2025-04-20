#pragma once
#include <vector>
#include "component.h"
#include "shader.h"
#include "camera.h"

Entity* createEntityFromModel(Model* model, ModelNode* node, std::vector<MeshRenderer*>* renderers, Entity* parentEntity, bool first, unsigned int nextEntityID);
void drawPickingScene(std::vector<MeshRenderer*>& renderers, Camera& camera, unsigned int pickingShader);
void drawScene(std::vector<MeshRenderer*>& renderers, Camera& camera, Entity* nodeClicked, bool enableDirLight, DirectionalLight* sun);
