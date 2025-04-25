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

layout (location = 5) uniform sampler2D texture_diffuse;
layout (location = 6) uniform sampler2D texture_specular;

layout (location = 0) in vec2 texCoord;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 fragPos;

layout (location = 4) uniform vec4 baseColor;
layout (location = 10) uniform vec3 viewPos;
layout (location = 9) uniform float shininess;
layout (location = 100) uniform DirectionalLight dirLight;
uniform PointLight pointLights[16];
uniform int numPointLights;
out vec4 FragColor;

vec4 diffuseColor;
vec4 specularColor;

vec3 applyDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 color, vec3 ambient);

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);

    float diff = max(dot(normal, lightDir), 0.0);

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);

    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
  			     light.quadratic * (distance * distance));    

    vec3 ambient  = light.ambient  * vec3(texture(texture_diffuse, texCoord));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(texture_diffuse, texCoord));
    vec3 specular = light.specular * spec * vec3(texture(texture_specular, texCoord));

    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;

    return (ambient + diffuse + specular);
} 

void main(){
     diffuseColor = texture(texture_diffuse, texCoord);

    if(diffuseColor.a < 0.1f){
        discard;
    }
    
    specularColor = texture(texture_specular, texCoord);

    vec3 norm = normalize(normal);
    vec3 viewDir = normalize(viewPos - fragPos);

    vec3 ambient = dirLight.ambient;
    diffuseColor *= baseColor; 
    vec4 finalColor;

    if(dirLight.enabled){
        finalColor = vec4(applyDirectionalLight(dirLight, norm, viewDir, vec3(baseColor.r, baseColor.g, baseColor.b), ambient), 1.0f);
    } else{
        //finalColor = diffuseColor;
    }
    
    for(int i = 0; i < numPointLights; i++){
        finalColor += vec4(CalcPointLight(pointLights[i], norm, fragPos, viewDir).rgb, 1.0f);
    }

    FragColor = finalColor;
}

vec3 applyDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 color, vec3 ambient){
    vec3 lightDir = normalize(light.position - fragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    ambient = ambient * vec3(diffuseColor);
    vec3 diffuse = light.diffuse * diff * vec3(diffuseColor);
    vec3 specular = light.specular * spec * vec3(specularColor.r);
    vec3 finalColor = (ambient + specular + diffuse);
    return finalColor;
}