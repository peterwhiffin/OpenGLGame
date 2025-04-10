#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

layout (location = 0) uniform mat4 model;
layout (location = 1) uniform mat4 view;
layout (location = 2) uniform mat4 projection;
layout (location = 3) uniform mat4 normalMatrix;

layout (location = 0) out vec2 texCoord;
layout (location = 1) out vec3 normal;
layout (location = 2) out vec3 fragPos;

void main(){
    fragPos = vec3(model * vec4(aPos, 1.0));
    normal = mat3(normalMatrix) * aNormal;
    texCoord = aTexCoord;
    gl_Position = projection * view * vec4(fragPos, 1.0);
}