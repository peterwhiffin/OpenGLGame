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

const float kernel5[5][5] = float[5][5](
    float[](1, 4, 7, 4, 1),
    float[](4, 16, 26, 16, 4),
    float[](7, 26, 41, 26, 7),
    float[](4, 16, 26, 16, 4),
    float[](1, 4, 7, 4, 1)
);

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

layout (location = 36) uniform PointLight pointLights[16];
layout (location = 120) uniform SpotLight spotLights[16];

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BloomColor;
layout (location = 2) out vec3 ViewPosition;
layout (location = 3) out vec3 ViewNormal;

float ShadowCalculation(int index, vec3 N)
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

    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            float pcfDepth = texture(spotLightShadowMaps[index], projCoords.xy + vec2(x, y) * texelSize).r;
            float weight = kernel5[x + 2][y + 2];
            shadow += (currentDepth - bias > pcfDepth ? 1.0 : 0.0) * weight;
            weightSum += weight;
        }
    }

    shadow /= weightSum;

    if(projCoords.z > 1.0){
        shadow = 0.0;
    }
 
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
    vec3 albedo = texture(albedoMap, fromVert.texCoord).rgb;
    vec3 normal = texture(normalMap, fromVert.texCoord).rgb;
    float roughness = texture(roughnessMap, fromVert.texCoord).r;
    float metallic = texture(metallicMap, fromVert.texCoord).r * metallicStrength;
    float ao = texture(aoMap, fromVert.texCoord).r * aoStrength;

    ViewPosition = fromVert.gPosition;
    ViewNormal = fromVert.gNormal;
    metallic = 0.1;
    ao = 1.0;

    vec3 N = normal * 2.0 - 1.0;
    N.xy *= normalStrength;
    N = normalize(N);
    N = fromVert.TBN * N;

    vec3 V = normalize(camPos - fromVert.fragPos);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);
	           
    vec3 Lo = vec3(0.0);
    
for(int i = 0; i < fromVert.numSpotLights; ++i) {
        /* if(spotLights[i].brightness == 0){
            continue;
        } */

        vec3 L = normalize(spotLights[i].position - fromVert.fragPos);
        vec3 H = normalize(V + L);
        float distance    = length(spotLights[i].position - fromVert.fragPos);
        float attenuation = (1.0 / (distance * distance));
        vec3 radiance     = spotLights[i].color * attenuation * spotLights[i].brightness;        

        float theta = dot(L, normalize(-spotLights[i].direction));
        float epsilon = spotLights[i].cutOff - spotLights[i].outerCutOff;
        float intensityRadiance = smoothstep(spotLights[i].outerCutOff, spotLights[i].cutOff, theta) * spotLights[i].brightness;

        radiance *= intensityRadiance;

        float shadowCalc = ShadowCalculation(i, N);
        
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
        vec3 L = normalize(pointLights[i].position - fromVert.fragPos);
        vec3 H = normalize(V + L);
        float distance    = length(pointLights[i].position - fromVert.fragPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance     = pointLights[i].color * attenuation;        
        
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
  
    vec3 ambient = vec3(0.045) * albedo * ao;
    vec3 color = ambient + Lo;
	
    FragColor = vec4(color * baseColor, 1.0);
    
    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));

    if(brightness > bloomThreshold){
        BloomColor = vec4(FragColor.rgb, 1.0);
    } else {
        BloomColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}  