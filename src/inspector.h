#pragma once

struct Scene;
struct Resources;
struct EditorState;
struct RenderState;

enum InspectorState {
    Empty,
    SceneEntity,
    Prefab,
    Resource
};

void buildInspector(Scene* scene, Resources* resources, RenderState* renderer, EditorState* editor);