#pragma once
#include <glad/glad.h>
#include <glm/vec2.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <Jolt/Jolt.h>
#include <Jolt/Math/Float2.h>
#include <Jolt/Renderer/DebugRendererSimple.h>

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

inline vec3 min(const vec3& a, const vec4& b) {
    return vec3(
        std::min(a.GetX(), b.GetX()),
        std::min(a.GetY(), b.GetY()),
        std::min(a.GetZ(), b.GetZ()));
}

inline vec3 max(const vec3& a, const vec4& b) {
    return vec3(
        std::max(a.GetX(), b.GetX()),
        std::max(a.GetY(), b.GetY()),
        std::max(a.GetZ(), b.GetZ()));
}