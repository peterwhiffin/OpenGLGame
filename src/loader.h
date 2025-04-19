#ifndef LOADER_H
#define LOADER_H

#include <vector>
#include <string>

#include "entity.h"

Model* loadModel(std::string path, std::vector<Texture>* allTextures, unsigned int shader);

unsigned int loadTextureFromFile(const char* path, bool gamma);

#endif