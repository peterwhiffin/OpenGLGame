#version 460 core

out vec4 FragColor;

in vec2 TexCoords;

layout (binding = 0) uniform sampler2D gPosition;
layout (binding = 1) uniform sampler2D gNormal;
layout (binding = 2) uniform sampler2D texNoise;
layout (binding = 3) uniform sampler2D bloomTex;

uniform vec3 samples[8];

int kernelSize = 8;

layout (location = 6) uniform float radius;
layout (location = 7) uniform float bias;
layout (location = 8) uniform vec2 noiseScale;
layout (location = 9) uniform float power;

 layout (std140, binding = 0) uniform global{
    mat4 view;
    mat4 projection;
}; 

void main()
{
    vec4 color = texture(bloomTex, TexCoords);
    vec3 fragPos = texture(gPosition, TexCoords).xyz;
    vec3 normal = normalize(texture(gNormal, TexCoords).rgb);
    vec3 randomVec = normalize(texture(texNoise, TexCoords * noiseScale).xyz);
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    float occlusion = 0.0;

    for(int i = 0; i < kernelSize; ++i)
    {
        // get sample position
        vec3 AsamplePos = TBN * samples[i]; // from tangent to view-space
        float NdotRay = dot(normal, normalize(AsamplePos));
        if(NdotRay < 0.5){
            continue;
        }

        float scale = abs(fragPos.z) / 1.0;  // Reference depth is something like 100.0f (tune it based on your scene)

        scale = clamp(scale, 0.2, 2.9); // Prevent it from scaling too drastically

        vec3 samplePos = fragPos + AsamplePos * radius * scale;
 
        vec4 offset = vec4(samplePos, 1.0);
        offset = projection * offset; // from view to clip-space
        offset.xyz /= offset.w; // perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0
        
        float sampleDepth = texture(gPosition, offset.xy).z; // get depth value of kernel sample
        
        float viewDistance = length(fragPos);
        float distance = abs(fragPos.z - sampleDepth);
        float normalizedDistance = distance / viewDistance;
        float rangeCheck = smoothstep(0.0, radius, radius - normalizedDistance * radius);
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;           
    }

    occlusion = 1.0 - (occlusion / kernelSize);

    color.a = pow(occlusion, power);
    FragColor = color;
}