#pragma once
#include "component.h"

void createContext(Scene* scene);
void initRenderer(Scene* scene);
void renderScene(Scene* scene);
void deleteBuffers(Scene* scene);

class MyDebugRenderer : public JPH::DebugRendererSimple {
   public:
    std::vector<DebugLine> lines;
    std::vector<DebugTri> triangles;

    void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;
    void DrawTriangle(JPH::RVec3Arg inV1, const JPH::RVec3Arg inV2, const JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow) override;
    void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight) override;
    void Clear();
};
