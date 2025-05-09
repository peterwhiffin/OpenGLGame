#version 460 core

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in ivec4 boneIds;
layout (location = 5) in vec4 weights;

layout (location = 4) uniform mat4 model;
// layout (location = 5) uniform mat3 normalMatrix;
layout (location = 6) uniform int numSpotLights;
layout (location = 7) uniform int numPointLights;
layout (location = 15) uniform mat4 lightSpaceMatrix[16];
// layout (location = 32) uniform mat3 gNormalMatrix;
uniform mat4 finalBoneMatrices[MAX_BONES];

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
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    
        
    vec4 totalPosition = vec4(0.0); 

    vec4 normalWeights = normalize(weights);
     
    for(int i = 0; i < MAX_BONE_INFLUENCE; i++){
        if(boneIds[i] == -1) 
            continue;

         if(boneIds[i] >= MAX_BONES) 
        {
            totalPosition = vec4(aPos,1.0f);
            break;
        }  

        vec4 localPosition = finalBoneMatrices[boneIds[i]] * vec4(aPos,1.0);
        totalPosition += localPosition * normalWeights[i];
        // vec3 localNormal = mat3(finalBonesMatrices[boneIds[i]]) * norm;   
    }

    vec4 modelPos = totalPosition;

    modelPos = model * totalPosition;

    // modelPos = model * finalBoneMatrices[boneIds[0]] * vec4(aPos, 1.0);
    // modelPos = model * vec4(aPos, 1.0);
    vec4 viewFrag = view * modelPos; 

    toFrag.fragPos = modelPos.xyz;
    toFrag.texCoord = aTexCoord;
    toFrag.normal = normalMatrix * aNormal; 
    toFrag.gNormal = mat3(view) * toFrag.normal;
    toFrag.numSpotLights = numSpotLights; 
    toFrag.numPointLights = numPointLights;
    toFrag.gPosition = viewFrag.xyz;

    gl_Position = projection * viewFrag;
 
    
    for(int i = 0; i < numSpotLights; i++){
        toFrag.fragPosLightSpace[i] = lightSpaceMatrix[i] * modelPos;
    }

    vec3 T = normalize(normalMatrix * aTangent);
    vec3 N = normalize(toFrag.normal);
    T = normalize(T - N * dot(N, T));
    vec3 B = cross(N, T); 
    
    if(dot(cross(T, N), B) < 0.0f){
        T = T * -1.0f;
    }

    toFrag.TBN = mat3(T, B, N);
}