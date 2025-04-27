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

const float PI = 3.14159265359;
const float epsilon = 1e-6f;

layout (location = 0) in vec2 TexCoords;
layout (location = 1) in vec3 WorldPos;
layout (location = 2) in vec3 Normal;
layout (location = 3) in mat3 TBN;

layout (binding = 0) uniform sampler2D albedoMap;
layout (binding = 1) uniform sampler2D roughnessMap;
layout (binding = 2) uniform sampler2D metallicMap;
layout (binding = 3) uniform sampler2D aoMap;
layout (binding = 4) uniform sampler2D normalMap;

// material parameters
layout (location = 8) uniform vec3 camPos; 
layout (location = 9) uniform vec3 baseColor;
layout (location = 10) uniform float metallicStrength;
layout (location = 11) uniform float roughnessStrength;
layout (location = 12) uniform float aoStrength;
layout (location = 13) uniform float normalStrength;
layout (location = 14) uniform float numPointLights;
layout (location = 15) uniform float bloomThreshold;

// lights
layout (location = 40) uniform DirectionalLight dirLight; 
layout (location = 48) uniform PointLight pointLights[16];

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BloomColor;

int kernelSize = 64;

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
    float k = (r*r) / 8.0;

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
    vec3 albedo = texture(albedoMap, TexCoords).rgb;
    vec3 normal = texture(normalMap, TexCoords).rgb;
    float roughness = texture(roughnessMap, TexCoords).r;
    float metallic = texture(metallicMap, TexCoords).r * metallicStrength;
    float ao = texture(aoMap, TexCoords).r * aoStrength;

    roughness *= 0.5;
    metallic = 0.1;
    ao = 0.0;

    vec3 N = normal * 2.0 - 1.0;
    N.xy *= normalStrength;
    N = normalize(N);
    N = TBN * N;

    // N = normalize(Normal);

    vec3 V = normalize(camPos - WorldPos);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);
	           
    // reflectance equation
    vec3 Lo = vec3(0.0);

    for(int i = 0; i < 1; ++i) {
        // calculate per-light radiance
        vec3 L = normalize(pointLights[i].position - WorldPos);
        vec3 H = normalize(V + L);
        float distance    = length(pointLights[i].position - WorldPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance     = pointLights[i].color * attenuation;        
        
        // cook-torrance brdf
        float NDF = DistributionGGX(N, H, roughness);        
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;	  
        
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular     = numerator / denominator;  
            
        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);                
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; 
    }   
  
    vec3 ambient = vec3(0.002) * albedo * ao;
    vec3 color = ambient + Lo;
	
    FragColor = vec4(color, 1.0);
    
    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));

    if(brightness > bloomThreshold){
        BloomColor = vec4(FragColor.rgb, 1.0);
    } else {
        BloomColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}  