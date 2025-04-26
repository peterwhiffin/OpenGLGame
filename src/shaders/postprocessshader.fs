#version 460 core

out vec4 FragColor;

layout (location = 2) in vec2 texCoord;

layout (location = 0, binding = 0) uniform sampler2D forwardBuffer;

void main(){
    vec3 forwardColor = texture(forwardBuffer, texCoord).rgb;
    FragColor = vec4(forwardColor, 1.0);
}
