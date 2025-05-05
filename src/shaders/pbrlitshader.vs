#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec3 aTangent;

//  layout (location = 4) uniform mat4 model;
// layout (location = 5) uniform mat4 view;
// layout (location = 6) uniform mat4 projection;
//  layout (location = 7) uniform mat3 normalMatrix;

layout (location = 0) out vec2 texCoord;
layout (location = 1) out vec3 fragPos;
layout (location = 2) out vec3 normal;
layout (location = 3) out mat3 TBN;

layout (std140, binding = 0) uniform matrices{
    mat4 view;
    mat4 projection;
};

layout (std140, binding = 1) uniform lighting{
    mat3 normalMatrix;
    mat4 model;
    int numSpotLights;
}; 

layout (std140, binding = 2) uniform lightMatrix{
    mat4 lightSpaceMatrix[16];
};

// uniform int maxSpotLights;
out vec4 fragPosLightSpace[16];
out vec3 gPosition;
out vec3 gNormal;

void main(){
    gPosition = (view * model * vec4(aPos, 1.0)).xyz;
    gNormal = transpose(inverse(mat3(view * model))) * aNormal;
    fragPos = vec3(model * vec4(aPos, 1.0));
    texCoord = aTexCoord;
    gl_Position = projection * view * vec4(fragPos, 1.0);
    normal = normalMatrix * aNormal; 
    
    for(int i = 0; i < numSpotLights; i++){
        fragPosLightSpace[i] = lightSpaceMatrix[i] * model * vec4(aPos, 1.0);
    }

    vec3 T = normalize(normalMatrix * aTangent);
    vec3 N = normalize(normalMatrix * aNormal);
    T = normalize(T - N * dot(N, T));
    vec3 B = cross(N, T); 
    
    if(dot(cross(T, N), B) < 0.0f){
        T = T * -1.0f;
    }

    TBN = mat3(T, B, N);
}