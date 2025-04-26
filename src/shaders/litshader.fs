#version 460 core

struct DirectionalLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    bool enabled;
};

struct PointLight {
    int isEnabled;
    vec3 position;
    
    float brightness;
    float constant;
    float linear;
    float quadratic;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

layout (location = 0, binding = 0) uniform sampler2D texture_diffuse;
layout (location = 1, binding = 1) uniform sampler2D texture_specular;
layout(location = 2, binding = 2) uniform sampler2D texture_normal;

layout (location = 13) in vec2 texCoord;
layout (location = 14) in vec3 normaly;
layout (location = 15) in vec3 fragPos;
layout (location = 16) in mat3 TBN;

layout (location = 8) uniform vec3 viewPos;
layout (location = 9) uniform vec4 baseColor;
layout (location = 10) uniform float shininess;
layout (location = 11) uniform float normalStrength;
layout (location = 12) uniform int numPointLights;
layout (location = 13) uniform float bloomThreshold;

layout (location = 32) uniform DirectionalLight dirLight;
layout (location = 37) uniform PointLight pointLights[16];

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BloomColor;

vec4 diffuseColor;
vec4 specularColor;

vec3 applyDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 ambient);

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float diff = max(dot(lightDir, normal), 0.0);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    

    attenuation = 1.0 / (distance * distance);
    vec3 ambient  = light.ambient  * vec3(diffuseColor);
    vec3 diffuse  = light.diffuse  * diff * vec3(diffuseColor);
    vec3 specular = light.specular * spec * vec3(specularColor.r);

    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;

    return (ambient + diffuse + specular);
} 

vec3 applyDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 ambient){
    vec3 lightDir = normalize(light.position - fragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float diff = max(dot(normal, lightDir), 0.0);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    ambient = ambient * vec3(diffuseColor);
    vec3 diffuse = light.diffuse * diff * vec3(diffuseColor);
    vec3 specular = light.specular * spec * vec3(specularColor.r);
    vec3 finalColor = (ambient + specular + diffuse);
    return finalColor;
}

void main(){
     diffuseColor = texture(texture_diffuse, texCoord);
    if(diffuseColor.a < 0.1f){
        discard;
    }
    
    specularColor = texture(texture_specular, texCoord);

    // vec3 norm = normalize(normal);
    vec3 norm1 = texture(texture_normal, texCoord).rgb;
    vec3 norm = norm1 * 2.0 - 1.0;
    norm.xy *= normalStrength;
    norm = normalize(norm);
    norm = TBN * norm;
 
    // vec3 norm = TBN * normalize(vec3(texture(texture_normal, texCoord).xy, 100.0f));

    // norm = normalize(normal);
    vec3 viewDir = normalize(viewPos - fragPos);

    vec3 ambient = dirLight.ambient;
    // diffuseColor *= baseColor; 
    vec4 finalColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);

    if(dirLight.enabled){
        finalColor = vec4(applyDirectionalLight(dirLight, norm, viewDir, ambient), 1.0f);
    } else{
        //finalColor = diffuseColor;
    }
    
    for(int i = 0; i < numPointLights; i++){
        vec3 next = CalcPointLight(pointLights[i], norm, fragPos, viewDir);
         finalColor += vec4(next.r, next.g, next.b, 1.0f);
    }

    FragColor = vec4(finalColor.rgb, 1.0f); 
 
    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));

    if(brightness > bloomThreshold){
        BloomColor = vec4(FragColor.rgb, 1.0);
    } else {
        BloomColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}

