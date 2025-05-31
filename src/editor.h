#pragma once
#include <unordered_set>
#include <unordered_map>

#include "utils/imgui.h"
#include "utils/mathutils.h"
#include "physics.h"
#include "ecs.h"

struct Scene;
struct Resources;
struct GLFWwindow;
enum InspectorState;

struct EditorCameraController {
    uint32_t entityID;
    uint32_t cameraTargetEntityID;
    uint32_t cameraEntityID;
    Camera* camera;
    float pitch = 0;
    float yaw = 0;
    float sensitivity = .3;
    float moveSpeed = 10;
};

enum EditorMode {
    Default,
    CameraControl,
    Picking
};

struct EditorState {
    double timeAccum = 0.0f;

    float FPS = 0.0f;
    float frameTime = 0.0f;
    float frameCount = 0;

    bool isPicking = false;
    bool canPick = true;
    bool canDelete = true;
    bool showDemoWindow = true;
    bool mouseInViewport = false;
    bool cameraActive = false;
    bool playing = false;

    ImVec2 viewportStart = ImVec2(0.0f, 0.0f);
    ImVec2 viewportEnd = ImVec2(0.0f, 0.0f);
    ImVec2 cursorPos = ImVec2(0.0f, 0.0f);
    ImVec2 windowPos = ImVec2(0.0f, 0.0f);

    vec3 worldPos = vec3(0.0f, 0.0f, 0.0f);
    Scene* playScene;
    EditorCameraController cameraController;
    EditorMode editorMode = Default;
    ContextMenuType contextType;
    InspectorState inspectorState;

    uint32_t nodeClicked = INVALID_ID;
    std::string fileClicked = "";

    std::unordered_set<uint32_t> selectedEntities;
};

void initEditor(EditorState* editor, GLFWwindow* window);
void checkPicker(Scene* scene, RenderState* renderer, EditorState* editor);
void updateAndDrawEditor(Scene* scene, RenderState* renderer, Resources* resources, EditorState* editor);
bool checkFilenameUnique(std::string path, std::string filename);
void destroyEditor();
void updateEditor(Scene* scene, Resources* resources, RenderState* renderer, EditorState* editor);