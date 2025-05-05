#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec3 aTangent;
layout (location = 1) in vec2 aTexCoord;

layout (location = 4) uniform mat4 model;
layout (location = 5) uniform mat4 view;
layout (location = 6) uniform mat4 projection;
layout (location = 7) uniform mat3 normalMatrix;

out vec3 FragPos;
out vec2 TexCoords;
out vec3 Normal;

void main()
{
    vec4 viewPos = view * model * vec4(aPos, 1.0);
    FragPos = viewPos.xyz; 
    TexCoords = aTexCoord;
    mat3 normalMat = transpose(inverse(mat3(view * model)));
    Normal = normalMat * aNormal;
    
    gl_Position = projection * viewPos;
}