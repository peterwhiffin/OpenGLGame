#version 460 core
layout (location = 0) in vec3 aPos;

layout (location = 1) uniform mat4 viewProjection;
layout (location = 2) uniform mat4 model;

void main()
{
    gl_Position = viewProjection * model * vec4(aPos, 1.0);
}