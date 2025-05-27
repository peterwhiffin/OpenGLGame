#pragma once
#include <unordered_set>
#include <unordered_map>

#include "utils/mathutils.h"
#include "physics.h"
#include "ecs.h"

struct Scene;
struct Resources;
struct GLFWwindow;
enum InspectorState;

struct EditorState {
    double timeAccum = 0.0f;

    float FPS = 0.0f;
    float frameTime = 0.0f;
    float frameCount = 0;

    bool isPicking = false;
    bool canPick = true;
    bool canDelete = true;
    bool showDemoWindow = true;

    ContextMenuType contextType;
    InspectorState inspectorState;

    uint32_t nodeClicked = INVALID_ID;
    std::string fileClicked = "";

    std::unordered_set<uint32_t> selectedEntities;
};

void initEditor(EditorState* editor, GLFWwindow* window);
void checkPicker(Scene* scene, RenderState* renderer, EditorState* editor, glm::dvec2 pickPosition);
void drawEditor(Scene* scene, RenderState* renderer, Resources* resources, EditorState* editor);
bool checkFilenameUnique(std::string path, std::string filename);
void destroyEditor();