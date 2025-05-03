#version 460 core

out vec4 FragColor;

layout (location = 2) in vec2 texCoord;

layout (binding = 0) uniform sampler2D forwardBuffer;
layout (binding = 1) uniform sampler2D bloomImage;
layout (binding = 2) uniform sampler2D depthTex;
layout (binding = 3) uniform sampler2D SSAOTex;

layout (location = 3) uniform float exposure;
layout (location = 4) uniform float bloomAmount;
layout (location = 5) uniform float aoAmount;
layout (location = 6) uniform vec3 pickingID;

void main(){
    vec3 forwardColor = texture(forwardBuffer, texCoord).rgb;
    vec4 bloomColor = texture(bloomImage, texCoord);
    float depth = texture(depthTex, texCoord).r;
    float ssao = texture(SSAOTex, texCoord).r;

    forwardColor += bloomColor.rgb * bloomAmount; 
    vec3 mapped = vec3(1.0) - exp(-forwardColor * exposure);
    mapped = pow(mapped , vec3(1.0 / 2.2));
    mapped *= ssao;
    FragColor = vec4(mapped , 1.0);
    // FragColor = vec4(vec3(ssao) , 1.0);
}