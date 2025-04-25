#version 460 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

layout (location = 0) in vec3 fragPos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;

layout (location = 5, binding = 0) uniform sampler2D texture_diffuse;
layout (location = 6, binding = 1) uniform sampler2D texture_specular;
layout (location = 14, binding = 2) uniform sampler2D texture_normal;

void main(){
    gPosition = fragPos;
    gNormal = normalize(normal);
    gAlbedoSpec.rgb = texture(texture_diffuse, texCoord).rgb;
    gAlbedoSpec.a = texture(texture_specular, texCoord).r; // calculate full spec here?;
}