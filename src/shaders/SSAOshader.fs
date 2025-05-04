#version 460 core

out vec4 FragColor;

in vec2 TexCoords;

layout (binding = 0) uniform sampler2D gPosition;
layout (binding = 1) uniform sampler2D gNormal;
layout (binding = 2) uniform sampler2D texNoise;
layout (binding = 3) uniform sampler2D bloomTex;

uniform vec3 samples[64];

// parameters (you'd probably want to use them as uniforms to more easily tweak the effect)
int kernelSize = 64;

layout (location = 5) uniform mat4 projection;
layout (location = 6) uniform float radius;
layout (location = 7) uniform float bias;
layout (location = 8) uniform vec2 noiseScale;
layout (location = 9) uniform float power;
// tile noise texture over screen based on screen dimensions divided by noise size


void main()
{
    vec4 color = texture(bloomTex, TexCoords);
    // get input for SSAO algorithm
    vec3 fragPos = texture(gPosition, TexCoords).xyz;
    vec3 normal = normalize(texture(gNormal, TexCoords).rgb);
    vec3 randomVec = normalize(texture(texNoise, TexCoords * noiseScale).xyz);
    // create TBN change-of-basis matrix: from tangent-space to view-space
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    // iterate over the sample kernel and calculate occlusion factor
    float occlusion = 0.0;
    for(int i = 0; i < kernelSize; ++i)
    {
        // get sample position
        vec3 AsamplePos = TBN * samples[i]; // from tangent to view-space
        vec3 samplePos = fragPos + AsamplePos * radius; 
        float test = dot(AsamplePos, samplePos);
        if(test < 0.15){
            continue;
        }
        
        // project sample position (to sample texture) (to get position on screen/texture)
        vec4 offset = vec4(samplePos, 1.0);
        offset = projection * offset; // from view to clip-space
        offset.xyz /= offset.w; // perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0
        
        // get sample depth
        float sampleDepth = texture(gPosition, offset.xy).z; // get depth value of kernel sample
        
        // range check & accumulate
        // float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        

        float rangeCheck= abs(fragPos.z - sampleDepth) < radius ? 1.0 : 0.0;

        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;           
    }
    occlusion = 1.0 - (occlusion / kernelSize);

    color.a = pow(occlusion, power);
    FragColor = color;
}