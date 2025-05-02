#version 460 core

out vec4 FragColor;

layout (location = 2) in vec2 texCoord;

layout (binding = 0) uniform sampler2D forwardBuffer;
layout (binding = 1) uniform sampler2D bloomImage;
layout (binding = 2) uniform sampler2D depthTex;
layout (binding = 3) uniform sampler2D pickingTex;

layout (location = 3) uniform float exposure;
layout (location = 4) uniform float bloomAmount;
layout (location = 5) uniform float aoAmount;
layout (location = 6) uniform vec3 pickingID;

void main(){
    vec3 forwardColor = texture(forwardBuffer, texCoord).rgb;
    vec3 bloomColor = texture(bloomImage, texCoord).rgb;
    float depth = texture(depthTex, texCoord).r;
    vec3 id = texture(pickingTex, texCoord).rgb;


    /* if(id == pickingID){
        forwardColor = vec3(0.0f, 1.0f, 0.0f);
    } */
    

    //forwardColor *= aoColor;
    forwardColor += bloomColor * bloomAmount; 
    
    // vec3 mapped = forwardColor / (forwardColor + vec3(1.0));
    vec3 mapped = vec3(1.0) - exp(-forwardColor * exposure);
    mapped = pow(mapped, vec3(1.0 / 2.2));

    FragColor = vec4(mapped, 1.0);
    // FragColor = vec4(id, 1.0);
}