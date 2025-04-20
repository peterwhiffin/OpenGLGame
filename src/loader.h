#pragma once
#include "component.h"

Model* loadModel(std::string path, std::vector<Texture>* allTextures, unsigned int shader);

unsigned int loadTextureFromFile(const char* path, bool gamma);