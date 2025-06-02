#version 460 core
precision highp float;

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
    int isEnabled;//6
    int shadows; //7
};

const float PI = 3.14159265359;
const float epsilon = 1e-6f;

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
layout (location = 4) out vec3 ambientColor;

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

uniform float u_LightRadiusUV    = 0.005;
uniform float u_BlockerSearchUV  = 0.0025;
uniform vec3 fogColor = vec3(1.0, 1.0, 1.0);
uniform float maxFogDistance = 65.0;
uniform float minFogDistance = 1.0;
uniform float fogDensity = 10.0;

float gradientNoise(in vec2 uv)
{
	return fract(52.9829189 * fract(dot(uv, vec2(0.06711056, 0.00583715))));
}

highp float remap(highp float value, highp float inputMin, highp float inputMax, highp float outputMin, highp float outputMax) {
  highp float normalizedValue = (value - inputMin) / (inputMax - inputMin);
  return normalizedValue * (outputMax - outputMin) + outputMin;
}

  float ShadowPCSS(int index, vec3 N)
{
    vec3 proj = fromVert.fragPosLightSpace[index].xyz /
                fromVert.fragPosLightSpace[index].w;
    proj = proj * 0.5 + 0.5;

    if (proj.z > 1.0) return 0.0;

    vec3  L       = normalize(-spotLights[index].direction);
    float bias    = max(0.05 * (1.0 - dot(normalize(fromVert.normal), L)), 0.005);

    vec2 texelSize      = 1.0 / textureSize(spotLightShadowMaps[index], 0);

    float avgBlockerDepth = 0.0;
    int   blockerCount    = 0;

    for (int i = 0; i < POISSON_SAMPLES; ++i)
    {
        vec2 offset = poissonDisk[i] * u_BlockerSearchUV;
        float sampleDepth = texture(spotLightShadowMaps[index], proj.xy + offset).r;

        if (sampleDepth < proj.z - bias)
        {
            avgBlockerDepth += sampleDepth;
            blockerCount++;
        }
    }

    if (blockerCount == 0)
        return 0.0;

    avgBlockerDepth /= float(blockerCount);

    float filterRadiusUV = ((proj.z - avgBlockerDepth) *  u_LightRadiusUV) / avgBlockerDepth;

    float shadow = 0.0;
    for (int i = 0; i < POISSON_SAMPLES; ++i)
    {
        vec2 offset = poissonDisk[i] * filterRadiusUV;
        float sampleDepth = texture(spotLightShadowMaps[index], proj.xy + offset).r;
        shadow += (proj.z - bias > sampleDepth) ? 1.0 : 0.0;
    }

    shadow /= float(POISSON_SAMPLES);
    
    return shadow;
}  

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
         if(spotLights[i].brightness == 0 || spotLights[i].isEnabled == 0){
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
        if(spotLights[i].shadows != 0){
            shadowCalc = ShadowPCSS(i, N);
        }

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
  
    float fragDistance = length(fromVert.fragPos - camPos);
    float fogFactor = remap(fragDistance, minFogDistance, maxFogDistance, 0, 1); 
    fogFactor = clamp(fogFactor, 0, 1);
    vec3 fog = fogColor * fogFactor * fogDensity;
    vec3 ambient = ambientBrightness * albedo * ao;
    vec3 color = (ambient + Lo) * baseColor;
    FragColor = vec4(color + fog, 1.0);

    float brightness = dot(FragColor.xyz, vec3(0.2126, 0.7152, 0.0722));

    if(brightness > bloomThreshold){
        BloomColor = vec4(FragColor.rgb, 1.0);
    } else {
        BloomColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}  