#pragma once
#include "component.h"

unsigned int loadShader(const char *vertexPath, const char *fragmentPath);

namespace vertex_attribute_location {
constexpr unsigned int kVertexPosition = 0;
constexpr unsigned int kVertexNormal = 1;
constexpr unsigned int kVertexTangent = 2;
constexpr unsigned int kVertexTexCoord = 3;

// post-process
constexpr unsigned int kPVertexPosition = 0;
constexpr unsigned int kPVertexTexCoord = 1;
}  // namespace vertex_attribute_location

namespace uniform_location {
// lit forward uniforms
constexpr unsigned int kModelMatrix = 4;
constexpr unsigned int kViewMatrix = 5;
constexpr unsigned int kProjectionMatrix = 6;
constexpr unsigned int kNormalMatrix = 7;
constexpr unsigned int kViewPos = 8;
constexpr unsigned int kBaseColor = 9;
constexpr unsigned int kShininess = 10;
constexpr unsigned int kNormalStrength = 11;
constexpr unsigned int kNumPointLights = 12;
constexpr unsigned int kDirectionalLight = 32;
constexpr unsigned int kPointLight = 37;

// lit forward samplers
constexpr unsigned int kTextureDiffuse = 0;
constexpr unsigned int kTextureSpecular = 1;
constexpr unsigned int kTextureNormal = 2;

// post-process uniforms
constexpr unsigned int kPTexCoord = 2;
//  post-process samplers
constexpr unsigned int kForwardBuffer = 0;

// texture units
constexpr unsigned int kTextureDiffuseUnit = 0;
constexpr unsigned int kTextureSpecularUnit = 1;
constexpr unsigned int kTextureNormalUnit = 2;
}  // namespace uniform_location