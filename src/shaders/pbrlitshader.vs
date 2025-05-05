#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec3 aTangent;

layout (location = 4) uniform mat4 model;
layout (location = 5) uniform mat3 normalMatrix;
layout (location = 6) uniform int numSpotLights;
layout (location = 7) uniform int numPointLights;
layout (location = 15) uniform mat4 lightSpaceMatrix[16];

layout (std140, binding = 0) uniform global{
    mat4 view;
    mat4 projection;
};

out VertToFrag{
    vec2 texCoord;
    vec3 fragPos;
    vec3 normal;
    mat3 TBN;
    vec3 gPosition;
    vec3 gNormal;
    vec4 fragPosLightSpace[16];
    flat int numSpotLights;
    flat int numPointLights;
} toFrag;

void main(){
    toFrag.gPosition = (view * model * vec4(aPos, 1.0)).xyz;
    toFrag.gNormal = transpose(inverse(mat3(view * model))) * aNormal;
    toFrag.fragPos = vec3(model * vec4(aPos, 1.0));
    toFrag.texCoord = aTexCoord;
    gl_Position = projection * view * vec4(toFrag.fragPos, 1.0);
    toFrag.normal = normalMatrix * aNormal; 
    toFrag.numSpotLights = numSpotLights; 
    toFrag.numPointLights = numPointLights;

    for(int i = 0; i < numSpotLights; i++){
        toFrag.fragPosLightSpace[i] = lightSpaceMatrix[i] * model * vec4(aPos, 1.0);
    }

    vec3 T = normalize(normalMatrix * aTangent);
    vec3 N = normalize(normalMatrix * aNormal);
    T = normalize(T - N * dot(N, T));
    vec3 B = cross(N, T); 
    
    if(dot(cross(T, N), B) < 0.0f){
        T = T * -1.0f;
    }

    toFrag.TBN = mat3(T, B, N);
}