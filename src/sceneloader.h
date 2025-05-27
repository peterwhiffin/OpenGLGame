#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>

// #include "forward.h"

constexpr char* scenePath = "../data/scenes/";

struct Scene;
struct TextureSettings;
struct Resources;
struct RenderState;

enum TokenType {
    Value,
    BlockOpen,
    ValueSeparator,
    BlockClose,
    EndOfFile
};

struct Token {
    TokenType type;
    std::string text;
    Token* nextToken;
};

struct ComponentBlock {
    std::string type;
    std::unordered_map<std::string, std::string> memberValueMap;
};

void loadScene(Scene* scene, Resources* resources);
bool findLastScene(Scene* scene);
void saveScene(Scene* scene, Resources* resources);
void loadMaterials(Scene* scene, Resources* resources, RenderState* renderer);
void loadResourceSettings(Scene* scene, Resources* resources, std::unordered_set<std::string>& metaPaths);
void writeMaterial(Scene* scene, Resources* resources, std::filesystem::path path);
void writeTextureSettings(Scene* scene, TextureSettings settings);