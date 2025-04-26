#version 460 core

out vec4 FragColor;

layout (location = 2) in vec2 texCoord;

layout (location = 0, binding = 0) uniform sampler2D forwardBuffer;
layout (location = 1, binding = 1) uniform sampler2D bloomImage;

layout (location = 3) uniform float exposure;

void main(){
    vec3 forwardColor = texture(forwardBuffer, texCoord).rgb;
    vec3 bloomColor = texture(bloomImage, texCoord).rgb;
    
    forwardColor += bloomColor; 
    
    // vec3 mapped = forwardColor / (forwardColor + vec3(1.0));
    vec3 mapped = vec3(1.0) - exp(-forwardColor * exposure);
    mapped = pow(mapped, vec3(1.0 / 2.2));

    FragColor = vec4(mapped, 1.0);
}
