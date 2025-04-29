#pragma once
#include <string>
#include <unordered_map>
#include "component.h"

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

void loadScene(Scene* scene, std::string path);
bool findLastScene(std::string* outScene);
void saveScene(Scene* scene);