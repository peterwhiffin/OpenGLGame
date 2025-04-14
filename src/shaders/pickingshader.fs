#version 460 core
layout (location = 4) uniform vec3 baseColor;

out vec4 FragColor;

void main(){
    FragColor = vec4(baseColor, 1.0f);
}