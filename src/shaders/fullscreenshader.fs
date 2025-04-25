#version 460 core

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

// uniform int map;

out vec4 FragColor;

in vec2 texCoord;

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

layout (location = 10) uniform vec3 viewPos;
layout (location = 100) uniform DirectionalLight dirLight;
uniform PointLight pointLights[16];
uniform int numPointLights;
// layout (location = 4) uniform vec4 baseColor;
vec4 diffuseColor;
vec3 fragPos;
vec3 normal;
layout (location = 9) uniform float shininess;

vec3 applyDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 ambient){
    vec3 lightDir = normalize(light.position - fragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    ambient = ambient * vec3(diffuseColor.rgb);
    vec3 diffuse = light.diffuse * diff * diffuseColor.rgb;
    vec3 specular = light.specular * spec * diffuseColor.a;
    vec3 finalColor = (ambient + specular + diffuse);
    return finalColor;
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);

    float diff = max(dot(normal, lightDir), 0.0);

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);

    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
  			     light.quadratic * (distance * distance));    

    vec3 ambient  = light.ambient  * diffuseColor.rgb;
    vec3 diffuse  = light.diffuse  * diff * diffuseColor.rgb;
    vec3 specular = light.specular * spec * diffuseColor.a;

    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;

    return (ambient + diffuse + specular);
} 

void main(){
    diffuseColor = texture(gAlbedoSpec, texCoord);
    fragPos = texture(gPosition, texCoord).rgb;
    normal = texture(gNormal, texCoord).rgb;

   /*  if(diffuseColor.a < 0.1f){
        discard;
    } */
    
    vec3 norm = normalize(normal);
    vec3 viewDir = normalize(viewPos - fragPos);

    vec3 ambient = dirLight.ambient;
    // diffuseColor *= baseColor; 
    vec4 finalColor;

    if(dirLight.enabled){
        finalColor = vec4(applyDirectionalLight(dirLight, norm, viewDir, ambient), 1.0f);
    } else{
        //  finalColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    }

     for(int i = 0; i < numPointLights; i++){
        vec3 next = CalcPointLight(pointLights[i], norm, fragPos, viewDir);
         finalColor += vec4(next.r, next.g, next.b, 1.0f);
    } 

/* if (any(isnan(finalColor))) {
    FragColor = vec4(1, 0, 1, 1); // bright magenta = NaN
    return;
} */
    FragColor = finalColor;
}

