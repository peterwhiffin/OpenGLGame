#ifndef SHADER_H
#define SHADER_H

namespace vertex_attribute_location {
constexpr unsigned int kVertexPosition = 0;
constexpr unsigned int kVertexNormal = 1;
constexpr unsigned int kVertexTexCoord = 2;
}  // namespace vertex_attribute_location

namespace sampler_location {
constexpr unsigned int kTextureDiffuse = 0;
constexpr unsigned int kTextureSpecular = 1;
constexpr unsigned int kTextureShadowMap = 2;
constexpr unsigned int kTextureNoise = 3;
}  // namespace sampler_location

namespace uniform_location {
constexpr unsigned int kModelMatrix = 0;
constexpr unsigned int kViewMatrix = 1;
constexpr unsigned int kProjectionMatrix = 2;
constexpr unsigned int kNormalMatrix = 3;
constexpr unsigned int kBaseColor = 3;
}  // namespace uniform_location

unsigned int
loadShader(const char *vertexPath, const char *fragmentPath);
#endif