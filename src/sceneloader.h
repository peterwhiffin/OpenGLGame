#pragma once
#include <string>
#include <unordered_map>
#include "forward.h"

constexpr char* scenePath = "../data/scenes/";

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

void loadScene(Scene* scene);
bool findLastScene(Scene* scene);
void saveScene(Scene* scene);