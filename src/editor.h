#pragma once
#include "component.h"

void initEditor(Scene* scene);
void checkPicker(Scene* scene, glm::dvec2 pickPosition);
void drawEditor(Scene* scene);
void destroyEditor();