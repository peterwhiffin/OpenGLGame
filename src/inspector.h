#pragma once

struct Scene;

enum InspectorState {
    Empty,
    SceneEntity,
    Prefab,
    Resource
};

void buildInspector(Scene* scene);