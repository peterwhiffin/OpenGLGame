#pragma once
#include <glm/glm.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <string>
#include <vector>
#include <unordered_map>

#include "component.h"

Model* loadModel(Scene* gameScene, std::string path, std::vector<Texture>* allTextures, unsigned int shader, bool whiteIsDefault);
// unsigned int loadTextureFromFile(const char* path, bool gamma, GLint filter);