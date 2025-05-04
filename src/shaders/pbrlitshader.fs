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
    vec3 position;
    vec3 direction;
    vec3 color;
    float brightness;
    bool isEnabled;

    float cutOff;
    float outerCutOff;
    sampler2D shadowMap;
};

const float PI = 3.14159265359;
const float epsilon = 1e-6f;

layout (location = 0) in vec2 TexCoords;
layout (location = 1) in vec3 WorldPos;
layout (location = 2) in vec3 Normal;
layout (location = 3) in mat3 TBN;
in vec4 fragPosLightSpace[16];

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
layout (location = 14) uniform int numPointLights;
layout (location = 16) uniform int numSpotLights;
layout (location = 15) uniform float bloomThreshold;

// lights
// layout (location = 40) uniform DirectionalLight dirLight; 
layout (location = 48) uniform PointLight pointLights[16];
uniform SpotLight spotLights[16];
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BloomColor;
layout (location = 2) out vec3 ViewPosition;
layout (location = 3) out vec3 ViewNormal;

in vec3 gPosition;
in vec3 gNormal;

float ShadowCalculation(int index, vec3 N)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace[index].xyz / fragPosLightSpace[index].w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(spotLights[index].shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // calculate bias (based on depth map resolution and slope)
    vec3 normal = normalize(Normal);
    // vec3 lightDir = normalize(spotLights[index].position - WorldPos);
    vec3 lightDir = normalize(-spotLights[index].direction);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    // check whether current frag pos is in shadow
    //  float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
    // PCF
      float shadow = 0.0;
     vec2 texelSize = 1.0 / textureSize(spotLights[index].shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    { 
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(spotLights[index].shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9; 
     
    //  float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
     if(projCoords.z > 1.0)
        shadow = 0.0;
 
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
    // float k = (r*r) / 8.0;
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
    vec3 albedo = texture(albedoMap, TexCoords).rgb;
    vec3 normal = texture(normalMap, TexCoords).rgb;
    float roughness = texture(roughnessMap, TexCoords).r;
    float metallic = texture(metallicMap, TexCoords).r * metallicStrength;
    float ao = texture(aoMap, TexCoords).r * aoStrength;

    ViewPosition = gPosition;
    ViewNormal = gNormal;
    // roughness = 0.9;
    metallic = 0.1;
    ao = 1.0;

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
    
for(int i = 0; i < numSpotLights; ++i) {
        // calculate per-light radiance
        vec3 L = normalize(spotLights[i].position - WorldPos);
        vec3 H = normalize(V + L);
        float distance    = length(spotLights[i].position - WorldPos);
        float attenuation = (1.0 / (distance * distance));
        vec3 radiance     = spotLights[i].color * attenuation * spotLights[i].brightness;        


        float theta = dot(L, normalize(-spotLights[i].direction));
        float epsilon = spotLights[i].cutOff - spotLights[i].outerCutOff;
        // float intensity = clamp(((theta - spotLights[i].outerCutOff) / epsilon), 0.0, 1.0);
        float intensityRadiance = smoothstep(spotLights[i].outerCutOff, spotLights[i].cutOff, theta) * spotLights[i].brightness;

        radiance *= intensityRadiance;

        float shadowCalc = ShadowCalculation(i, N);
        
        if(spotLights[i].brightness == 0){
            shadowCalc = 0.0;
        }
        
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
        //  specular *= intensity;    
        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);                
        Lo += (kD * albedo / PI + specular) * radiance * NdotL * (1.0 - shadowCalc); 
    }


      for(int i = 0; i < numPointLights; ++i) {
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