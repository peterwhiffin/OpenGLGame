#pragma once
#include "utils/mathutils.h"
struct Scene;

void initEditor(Scene* scene);
void checkPicker(Scene* scene, glm::dvec2 pickPosition);
void drawEditor(Scene* scene);
bool checkFilenameUnique(std::string path, std::string filename);
void destroyEditor();