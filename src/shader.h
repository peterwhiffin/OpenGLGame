#ifndef SHADER_H
#define SHADER_H

namespace vertex_attribute_location {
constexpr unsigned int kVertexPosition = 0;
constexpr unsigned int kVertexNormal = 1;
constexpr unsigned int kVertexTexCoord = 2;
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
constexpr unsigned int kTextureShadowMap = 7;
constexpr unsigned int kTextureNoise = 8;

constexpr unsigned int kTextureDiffuseUnit = 0;
constexpr unsigned int kTextureSpecularUnit = 1;
constexpr unsigned int kTextureShadowMapUnit = 2;
constexpr unsigned int kTextureNoiseUnit = 3;

constexpr unsigned int kDirectionalLight = 100;
}  // namespace uniform_location

unsigned int loadShader(const char *vertexPath, const char *fragmentPath);
#endif