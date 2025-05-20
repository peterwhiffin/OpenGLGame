#version 460 core

out vec4 FragColor;

layout (location = 2) in vec2 texCoord;

layout (binding = 0) uniform sampler2D colorTex;
layout (binding = 1) uniform sampler2D bloomTex;

layout (location = 3) uniform float exposure;
layout (location = 4) uniform float bloomAmount;
layout (location = 5) uniform float aoAmount;

void main(){
    vec3 forwardColor = texture(colorTex, texCoord).rgb;
    
    vec4 bloomColor = texture(bloomTex, texCoord);

    forwardColor += bloomColor.rgb * bloomAmount; 
    vec3 mapped = vec3(1.0) - exp(-forwardColor * exposure);
    mapped = pow(mapped , vec3(1.0 / 2.2));
    mapped *= bloomColor.a;
    FragColor = vec4(mapped , 1.0);
}