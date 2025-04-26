#pragma once
#include "component.h"

unsigned int loadShader(const char *vertexPath, const char *fragmentPath);

namespace vertex_attribute_location {
constexpr unsigned int kVertexPosition = 0;
constexpr unsigned int kVertexNormal = 1;
constexpr unsigned int kVertexTangent = 2;
constexpr unsigned int kVertexTexCoord = 3;
}  // namespace vertex_attribute_location

namespace uniform_location {
constexpr unsigned int kModelMatrix = 0;
constexpr unsigned int kViewMatrix = 1;
constexpr unsigned int kProjectionMatrix = 2;
constexpr unsigned int kNormalMatrix = 3;
constexpr unsigned int kBaseColor = 4;
constexpr unsigned int kShininess = 9;
constexpr unsigned int kViewPos = 10;

constexpr unsigned int kTextureDiffuse = 5;
constexpr unsigned int kTextureSpecular = 6;
constexpr unsigned int kTextureNormal = 14;
constexpr unsigned int kTextureShadowMap = 7;
constexpr unsigned int kTextureNoise = 8;
constexpr unsigned int kGBufferTexPosition = 11;
constexpr unsigned int kGBufferTexNormal = 12;
constexpr unsigned int kGBufferTexAlbedoSpec = 13;
constexpr unsigned int kNormalStrength = 15;

constexpr unsigned int kTextureDiffuseUnit = 0;
constexpr unsigned int kTextureSpecularUnit = 1;
constexpr unsigned int kTextureNormalUnit = 2;
constexpr unsigned int kTextureShadowMapUnit = 3;
constexpr unsigned int kTextureNoiseUnit = 4;

constexpr unsigned int kDirectionalLight = 30;
constexpr unsigned int kPointLight = 64;
}  // namespace uniform_location