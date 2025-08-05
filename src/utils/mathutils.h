#pragma once
#include <Jolt/Jolt.h>
#include <Jolt/Math/Float2.h>
#include <Jolt/Renderer/DebugRendererSimple.h>
#include <cmath>
#include <glad/glad.h>
#include <glm/ext/quaternion_float.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/vec2.hpp>

using namespace JPH::literals;
using vec2 = JPH::Float2;
using vec3 = JPH::Vec3;
using vec4 = JPH::Vec4;
using quat = JPH::Quat;
using mat4 = JPH::Mat44;
using color = JPH::Color;

inline float lerp(const float a, const float b, float t) {
  return a + (b - a) * t;
}

inline vec3 lerp(const vec3 a, const vec3 b, float t) {
  return a + (b - a) * t;
}

inline vec3 min(const vec3 &a, const vec4 &b) {
  return vec3(std::min(a.GetX(), b.GetX()), std::min(a.GetY(), b.GetY()),
              std::min(a.GetZ(), b.GetZ()));
}

inline vec3 max(const vec3 &a, const vec4 &b) {
  return vec3(std::max(a.GetX(), b.GetX()), std::max(a.GetY(), b.GetY()),
              std::max(a.GetZ(), b.GetZ()));
}

inline float remap(float value, float inputMin, float inputMax, float outputMin,
                   float outputMax) {
  float normalizedValue = (value - inputMin) / (inputMax - inputMin);
  return normalizedValue * (outputMax - outputMin) + outputMin;
}

inline float vec3Distance(vec3 a, vec3 b) {
  float x = a.GetX() - b.GetX();
  float y = a.GetY() - b.GetY();
  float z = a.GetZ() - b.GetZ();

  return sqrt((x * x) + (y * y) + (z * z));
}
