#pragma once
#include "utils/mathutils.h"
struct Scene;

struct EditorInfo {
};

void initEditor(Scene* scene);
void checkPicker(Scene* scene, glm::dvec2 pickPosition);
void drawEditor(Scene* scene);
bool checkFilenameUnique(std::string path, std::string filename);
void destroyEditor();