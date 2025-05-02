#version 460 core
layout (location = 9) uniform vec3 baseColor;

out vec4 FragColor;

void main(){
    FragColor = vec4(baseColor, 1.0f);
    // FragColor = vec4(0.0, 1.0, 0.0, 1.0);
}