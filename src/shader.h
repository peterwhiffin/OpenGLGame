#pragma once
#include "component.h"

unsigned int loadShader(const char *vertexPath, const char *fragmentPath);

namespace vertex_attribute_location {
constexpr unsigned int kVertexPosition = 0;
constexpr unsigned int kVertexTexCoord = 1;
constexpr unsigned int kVertexNormal = 2;
constexpr unsigned int kVertexTangent = 3;
}  // namespace vertex_attribute_location

namespace uniform_location {
// lit forward uniforms
constexpr unsigned int kModelMatrix = 4;
constexpr unsigned int kViewMatrix = 5;
constexpr unsigned int kProjectionMatrix = 6;
constexpr unsigned int kNormalMatrix = 7;
constexpr unsigned int kViewPos = 8;
constexpr unsigned int kColor = 9;
constexpr unsigned int kRoughnessStrength = 10;
constexpr unsigned int kMetallicStrength = 11;
constexpr unsigned int kAOStrength = 12;
constexpr unsigned int kNormalStrength = 13;
constexpr unsigned int kNumPointLights = 14;
constexpr unsigned int kNumSpotLights = 16;
constexpr unsigned int kBloomThreshold = 15;
constexpr unsigned int kAORadius = 30;
constexpr unsigned int kAOBias = 31;
constexpr unsigned int kAOAmount = 5;
constexpr unsigned int kDirectionalLight = 40;
constexpr unsigned int kPointLight = 48;
constexpr unsigned int kSpotLight = 116;
constexpr unsigned int kSSAOKernel = 132;
// pbr uniforms

constexpr unsigned int kInvProjectionMatrix = 5;
// post-process uniforms
constexpr unsigned int kPExposure = 3;
constexpr unsigned int kPBloomAmount = 4;
// blur uniforms
constexpr unsigned int kBHorizontal = 3;
// texture units
constexpr unsigned int kTextureAlbedoUnit = 0;
constexpr unsigned int kTextureRoughnessUnit = 1;
constexpr unsigned int kTextureMetallicUnit = 2;
constexpr unsigned int kTextureAOUnit = 3;
constexpr unsigned int kTextureNormalUnit = 4;
constexpr unsigned int kTextureSSAOUnit = 5;

constexpr unsigned int kTextureSSAONoiseUnit = 0;
constexpr unsigned int kTextureDepthUnit = 1;
}  // namespace uniform_location