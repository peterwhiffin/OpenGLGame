#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 4) in ivec4 boneIds;
layout (location = 5) in vec4 weights;

layout (location = 1) uniform mat4 viewProjection;
layout (location = 2) uniform mat4 model;

uniform mat4 finalBoneMatrices[100];


void main()
{
    vec4 totalPosition = vec4(0.0);

    for(int i = 0; i < 4; i++){
        if(boneIds[i] == -1) 
            continue;

         if(boneIds[i] >= 100) 
        {
            totalPosition = vec4(aPos, 1.0);
            break;
        }  

        vec4 localPosition = finalBoneMatrices[boneIds[i]] * vec4(aPos,1.0);
        totalPosition += localPosition * weights[i];
    }

    vec4 modelPos = model * totalPosition;
    // gl_Position = viewProjection * model * vec4(aPos, 1.0);
    gl_Position = viewProjection * modelPos;
}