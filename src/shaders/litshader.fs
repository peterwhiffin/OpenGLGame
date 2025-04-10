#version 460 core

in vec2 texCoord;
in vec3 normal;
in vec3 fragPos;

uniform sampler2D texture_diffuse;

out vec4 FragColor;

void main(){
    FragColor = texture(texture_diffuse, texCoord);
//    FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
}