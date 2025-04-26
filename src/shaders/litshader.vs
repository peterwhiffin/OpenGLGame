#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTangent;
layout (location = 3) in vec2 aTexCoord;

layout (location = 4) uniform mat4 model;
layout (location = 5) uniform mat4 view;
layout (location = 6) uniform mat4 projection;
layout (location = 7) uniform mat3 normalMatrix;

layout (location = 13) out vec2 texCoord;
layout (location = 14) out vec3 normaly;
layout (location = 15) out vec3 fragPos;
layout (location = 16) out mat3 TBN;

void main(){
    fragPos = vec3(model * vec4(aPos, 1.0));
    normaly = normalMatrix * aNormal;
    texCoord = aTexCoord;
    gl_Position = projection * view * vec4(fragPos, 1.0);
    
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 N = normalize(normalMatrix * aNormal);
    T = normalize(T - N * dot(N, T));
    vec3 B = cross(N, T); 
    
    if(dot(cross(T, N), B) < 0.0f){
        T = T * -1.0f;
    }

    TBN = mat3(T, B, N);
}