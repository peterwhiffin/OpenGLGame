#version 460 core

struct DirectionalLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    bool enabled;
};

struct PointLight {
    vec3 position;
    vec3 color;
    float brightness;
    int isEnabled;
};

struct SpotLight{
    vec3 position;//0
    vec3 direction;//1
    vec3 color;//2
    float brightness;//3
    float cutOff;//4
    float outerCutOff;//5
    bool isEnabled;//6
};

const float PI = 3.14159265359;
const float epsilon = 1e-6f;

const float kernel3[3][3] = float[3][3](
    float[](1.0, 2.0, 1.0),
    float[](2.0, 4.0, 2.0),
    float[](1.0, 2.0, 1.0)
);  

/* const float kernel5[5][5] = float[5][5](
    float[](1, 4, 7, 4, 1),
    float[](4, 16, 26, 16, 4),
    float[](7, 26, 41, 26, 7),
    float[](4, 16, 26, 16, 4),
    float[](1, 4, 7, 4, 1)
); */

/* const int POISSON_SAMPLE_COUNT = 16;
const vec2 poissonDisk[POISSON_SAMPLE_COUNT] = vec2[](
    vec2(-0.94201624, -0.39906216),
    vec2(0.94558609, -0.76890725),
    vec2(-0.094184101, -0.92938870),
    vec2(0.34495938, 0.29387760),
    vec2(-0.91588581, 0.45771432),
    vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543, 0.27676845),
    vec2(0.97484398, 0.75648379),
    vec2(0.44323325, -0.97511554),
    vec2(0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023),
    vec2(0.79197514, 0.19090188),
    vec2(-0.24188840, 0.99706507),
    vec2(-0.81409955, 0.91437590),
    vec2(0.19984126, 0.78641367),
    vec2(0.14383161, -0.14100790)
); */

in VertToFrag{
    vec2 texCoord;
    vec3 fragPos;
    vec3 normal;
    mat3 TBN;
    vec3 gPosition;
    vec3 gNormal;
    vec4 fragPosLightSpace[16];
    flat int numSpotLights;
    flat int numPointLights;
} fromVert;

layout (binding = 0) uniform sampler2D albedoMap;
layout (binding = 1) uniform sampler2D roughnessMap;
layout (binding = 2) uniform sampler2D metallicMap;
layout (binding = 3) uniform sampler2D aoMap;
layout (binding = 4) uniform sampler2D normalMap;
// layout (binding = 5) uniform sampler2D spotLightShadowMaps[16];
layout (binding = 5) uniform sampler2D spotLightShadowMaps[16];

layout (location = 8) uniform vec3 camPos;
layout (location = 9) uniform float bloomThreshold;
layout (location = 10) uniform float metallicStrength; 
layout (location = 11) uniform float roughnessStrength; 
layout (location = 12) uniform float aoStrength; 
layout (location = 13) uniform float normalStrength; 
layout (location = 14) uniform vec3 baseColor; 
layout (location = 35) uniform float ambientBrightness; 
uniform vec2 textureTiling;

layout (location = 36) uniform PointLight pointLights[16];
layout (location = 120) uniform SpotLight spotLights[16];

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BloomColor;
layout (location = 2) out vec3 ViewPosition;
layout (location = 3) out vec3 ViewNormal;


// ─────────────────────────────────────────────────────
// CONSTANTS & UNIFORMS
// ─────────────────────────────────────────────────────
const int POISSON_SAMPLES = 16;
const vec2 poissonDisk[POISSON_SAMPLES] = vec2[](
    vec2(-0.94201624, -0.39906216),  vec2(0.94558609, -0.76890725),
    vec2(-0.09418410, -0.92938870),  vec2(0.34495938,  0.29387760),
    vec2(-0.91588581,  0.45771432),  vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543,  0.27676845),  vec2(0.97484398,  0.75648379),
    vec2(0.44323325, -0.97511554),   vec2(0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023),  vec2(0.79197514,  0.19090188),
    vec2(-0.24188840,  0.99706507),  vec2(-0.81409955,  0.91437590),
    vec2(0.19984126,  0.78641367),   vec2(0.14383161, -0.14100790)
);

// Tune these per-light or as global uniforms
uniform float u_LightRadiusUV    = 0.005;   // light size in shadow-map UVs
uniform float u_BlockerSearchUV  = 0.0025;  // search radius in UVs

/* float ShadowPCSS(int index, vec3 N)
{
    // 1. Project fragment into light clip space ➜ [0,1] UV + depth
    vec3 proj = fromVert.fragPosLightSpace[index].xyz /
                fromVert.fragPosLightSpace[index].w;
    proj = proj * 0.5 + 0.5; // NDC → UV

    // Outside of shadow map = lit
    if (proj.z > 1.0) return 0.0;

    // Bias to fight acne
    vec3  L    = normalize(-spotLights[index].direction);
    float bias = max(0.05 * (1.0 - dot(normalize(N), L)), 0.005);

    vec2 texelSize = 1.0 / textureSize(spotLightShadowMaps[index], 0);
    // ── 2. Blocker search ────────────────────────────
    float avgBlockerDepth = 0.0;
    int   blockerCount    = 0;

    for (int i = 0; i < POISSON_SAMPLES; ++i)
    {
        vec2 offset = poissonDisk[i] * u_BlockerSearchUV;
        float sampleTime = texture(spotLightShadowMaps[index], vec3(proj.xy + offset, proj.z - bias));

        if (sampleTime < 1.0) // shadowed → blocker exists
        {
            float sampleDepth = proj.z - bias - texelSize.y; // Approximation
            avgBlockerDepth += sampleDepth;
            blockerCount++;
        }
    }

    if (blockerCount == 0)
        return 0.0;

    avgBlockerDepth /= float(blockerCount);

    // ── 3. Penumbra size ─────────────────────────────
    float penumbra = (proj.z - avgBlockerDepth) / avgBlockerDepth;
    float filterRadiusUV = penumbra * u_LightRadiusUV;

    // ── 4. PCF filter with variable radius ───────────
    float shadow = 0.0;
    for (int i = 0; i < POISSON_SAMPLES; ++i)
    {
        vec2 offset = poissonDisk[i] * filterRadiusUV;
        float sampleTime = texture(spotLightShadowMaps[index], vec3(proj.xy + offset, proj.z - bias));
        shadow += 1.0 - sampleTime; // sample = 1.0 if lit, 0.0 if shadowed
    }

    shadow /= float(POISSON_SAMPLES);
    return shadow;
} */
// ─────────────────────────────────────────────────────
// PCSS SHADOW FUNCTION
// ─────────────────────────────────────────────────────
  float ShadowPCSS(int index, vec3 N)
{
    // 1. Project fragment into light clip space ➜ [0,1] UV + depth
    vec3 proj = fromVert.fragPosLightSpace[index].xyz /
                fromVert.fragPosLightSpace[index].w;
    proj = proj * 0.5 + 0.5;    // NDC → UV

    // Outside of shadow map = lit
    if (proj.z > 1.0) return 0.0;

    // Bias to fight acne (same you used before)
    vec3  L       = normalize(-spotLights[index].direction);
    float bias    = max(0.05 * (1.0 - dot(normalize(fromVert.normal), L)), 0.005);

    // sampler2D shadowTex = spotLightShadowMaps[index];
    vec2 texelSize      = 1.0 / textureSize(spotLightShadowMaps[index], 0);

    // ── 2. Blocker search ────────────────────────────
    float avgBlockerDepth = 0.0;
    int   blockerCount    = 0;

    for (int i = 0; i < POISSON_SAMPLES; ++i)
    {
        vec2 offset = poissonDisk[i] * u_BlockerSearchUV;
        float sampleDepth = texture(spotLightShadowMaps[index], proj.xy + offset).r;

        if (sampleDepth < proj.z - bias)   // blocker = nearer than receiver
        {
            avgBlockerDepth += sampleDepth;
            blockerCount++;
        }
    }

    // No blockers → fully lit
    if (blockerCount == 0)
        return 0.0;

    avgBlockerDepth /= float(blockerCount);

    // ── 3. Penumbra size (filter radius) ─────────────
    float penumbra = (proj.z - avgBlockerDepth) / avgBlockerDepth;


    float filterRadiusUV = penumbra * u_LightRadiusUV;   // in UV space

    // ── 4. PCF filter with variable radius ───────────
    float shadow = 0.0;
    for (int i = 0; i < POISSON_SAMPLES; ++i)
    {
        vec2 offset = poissonDisk[i] * filterRadiusUV;
        float sampleDepth = texture(spotLightShadowMaps[index], proj.xy + offset).r;
        shadow += (proj.z - bias > sampleDepth) ? 1.0 : 0.0;
    }

    shadow /= float(POISSON_SAMPLES);
    
// Already in proj.xy (UV from 0–1)
//  float distFromCenter = length(proj.xy - 0.5);
// float coneFade = smoothstep(1.0, 0.1, distFromCenter); // adjust 0.85 for edge softness
// shadow *= coneFade; // fade shadow toward outer cone 

    return shadow;
}  
/* float ShadowCalculation(int index, vec3 N)
{
    vec3 projCoords = fromVert.fragPosLightSpace[index].xyz / fromVert.fragPosLightSpace[index].w;
    projCoords = projCoords * 0.5 + 0.5;

    float currentDepth = projCoords.z;
    if (currentDepth > 1.0) return 0.0;

    vec3 normal = normalize(fromVert.normal);
    vec3 lightDir = normalize(-spotLights[index].direction);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(spotLightShadowMaps[index], 0);

    for (int i = 0; i < POISSON_SAMPLE_COUNT; ++i) {
        vec2 offset = poissonDisk[i] * texelSize * 2.0; // tweak scale for softness
        float sampledDepth = texture(spotLightShadowMaps[index], projCoords.xy + offset).r;
        shadow += (currentDepth - bias > sampledDepth) ? 1.0 : 0.0;
    }

    shadow /= float(POISSON_SAMPLE_COUNT);
    return shadow;
} */

 /* float ShadowCalculation(int index, vec3 N)
{
    vec3 projCoords = fromVert.fragPosLightSpace[index].xyz / fromVert.fragPosLightSpace[index].w;
    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = texture(spotLightShadowMaps[index], projCoords.xy).r; 
    float currentDepth = projCoords.z;
    vec3 normal = normalize(fromVert.normal);
    vec3 lightDir = normalize(-spotLights[index].direction);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(spotLightShadowMaps[index], 0);

    float weightSum = 0.0;

    // shadow += currentDepth - bias > closestDepth ? 1.0 : 0.0;

    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(spotLightShadowMaps[index], projCoords.xy + vec2(x, y) * texelSize).r;
            float weight = kernel3[x + 1][y + 1];
            shadow += (currentDepth - bias > pcfDepth ? 1.0 : 0.0) * weight;
            weightSum += weight;
        }
    }

    shadow /= weightSum;

    if(projCoords.z > 1.0){
        shadow = 0.0;
    }
 
    return shadow;
}  */  


float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return max(num / max(denom, 0.001), epsilon);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) * 0.125;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), epsilon);
    float NdotL = max(dot(N, L), epsilon);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

void main() {		
    vec2 tiledTexCoord = fromVert.texCoord * textureTiling;
    vec3 albedo = texture(albedoMap, tiledTexCoord).rgb * baseColor;
    vec3 normal = texture(normalMap, tiledTexCoord).rgb;
    float roughness = texture(roughnessMap, tiledTexCoord).r * roughnessStrength;
    float metallic = texture(metallicMap, tiledTexCoord).r * metallicStrength;
    float ao = texture(aoMap, tiledTexCoord).r * aoStrength;

    ViewPosition = fromVert.gPosition;
    ViewNormal = fromVert.gNormal;
    // metallic = 0.1;
    // ao = 1.0;

    vec3 N = normal * 2.0 - 1.0;
    N.xy *= normalStrength;
    N = normalize(N);
    N = normalize(fromVert.TBN * N);

    vec3 V = normalize(camPos - fromVert.fragPos);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);
	           
    vec3 Lo = vec3(0.0);
    
    float shadowCalc = 0.0;

for(int i = 0; i < fromVert.numSpotLights; ++i) {
         if(spotLights[i].brightness == 0){
            continue;
        } 

        vec3 L = normalize(spotLights[i].position - fromVert.fragPos);
        vec3 H = normalize(V + L);
        float distance    = length(spotLights[i].position - fromVert.fragPos);
        float attenuation = (1.0 / (distance * distance));
        vec3 radiance     = spotLights[i].color * attenuation * spotLights[i].brightness;        

        float theta = dot(L, normalize(-spotLights[i].direction));
        float epsilon = spotLights[i].cutOff - spotLights[i].outerCutOff;
        float intensityRadiance = smoothstep(spotLights[i].outerCutOff, spotLights[i].cutOff, theta) * spotLights[i].brightness;

        radiance *= intensityRadiance;

        // shadowCalc = ShadowCalculation(i, N);
        
        shadowCalc = ShadowPCSS(i, N);
        float NDF = DistributionGGX(N, H, roughness);        
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;	  
        
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular     = numerator / denominator;  
        float NdotL = max(dot(N, L), 0.0);                
        Lo += (kD * albedo / PI + specular) * radiance * NdotL * (1.0 - shadowCalc); 
    }


      for(int i = 0; i < fromVert.numPointLights; ++i) {
        if(pointLights[i].brightness == 0){
            continue;
        }

        vec3 L = normalize(pointLights[i].position - fromVert.fragPos);
        vec3 H = normalize(V + L);
        float distance    = length(pointLights[i].position - fromVert.fragPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance     = pointLights[i].color * attenuation * pointLights[i].brightness;        
        
        float NDF = DistributionGGX(N, H, roughness);        
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;	  
        
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular     = numerator / denominator;  
            
        float NdotL = max(dot(N, L), 0.0);                
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; 
    }     
  
    vec3 ambient = ambientBrightness * albedo * ao;
    vec3 color = ambient + Lo;
	
    FragColor = vec4(color * baseColor, 1.0);
    
    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));

    if(brightness > bloomThreshold){
        BloomColor = vec4(FragColor.rgb, 1.0);
    } else {
        BloomColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}  