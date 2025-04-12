#version 460 core
layout (location = 4) uniform vec4 baseColor;

out vec4 FragColor;


void main(){
    FragColor = baseColor;
}