#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include "physics.h"
#include "ecs.h"
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
// 16023
struct Token {
    TokenType type;
    std::string text;
    Token* nextToken;
};

struct ComponentBlock {
    std::string type;
    std::unordered_map<std::string, std::string> memberValueMap;
};

void loadScene(Resources* resources, Scene* scene);
void loadFirstFoundScene(Scene* scene, Resources* resources);
bool findLastScene(Scene* scene);
void saveScene(Scene* scene, Resources* resources);
void loadMaterials(Resources* resources, RenderState* renderer);
void loadResourceSettings(Resources* resources, std::unordered_set<std::string>& metaPaths);
void writeMaterial(Resources* resources, std::filesystem::path path);
void writeTextureSettings(TextureSettings settings);
void writeTempScene(Scene* scene);
void loadTempScene(Resources* resources, Scene* scene);
void loadPrefab(Resources* resources, std::filesystem::path path);
void initializeRigidbody(RigidBody* rb, PhysicsScene* physicsScene, EntityGroup* entities);
std::string writeNewPrefab(Scene* scene, uint32_t entityID);