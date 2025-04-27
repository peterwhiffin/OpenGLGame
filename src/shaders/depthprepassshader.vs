#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec3 aTangent;
layout (location = 1) in vec2 aTexCoord;

layout (location = 4) uniform mat4 model;
layout (location = 5) uniform mat4 view;
layout (location = 6) uniform mat4 projection;

void main()
{
    vec3 fragPos = vec3(model * vec4(aPos, 1.0));
     gl_Position = projection * view * vec4(fragPos, 1.0); 
}  