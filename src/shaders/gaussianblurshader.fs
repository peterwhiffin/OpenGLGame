#version 460 core
out vec4 FragColor;
  
layout (location = 2) in vec2 TexCoords;

layout (location = 0, binding = 0) uniform sampler2D image;
  
layout (location = 3) uniform bool horizontal;
layout (location = 4) uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{             
      vec2 tex_offset = 1.0 / textureSize(image, 0); // gets size of single texel
    vec4 result = texture(image, TexCoords) * weight[0]; // current fragment's contribution
    if(horizontal)
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(image, TexCoords + vec2(tex_offset.x * i, 0.0)) * weight[i];
            result += texture(image, TexCoords - vec2(tex_offset.x * i, 0.0)) * weight[i];
        }
    }
    else
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(image, TexCoords + vec2(0.0, tex_offset.y * i)) * weight[i];
            result += texture(image, TexCoords - vec2(0.0, tex_offset.y * i)) * weight[i];
        }
    }

    FragColor = result;  
 /* 
    vec2 tex_offset = 1.0 / textureSize(image, 0); // gets size of single texel
    vec4 result = texture(image, TexCoords); // current fragment's contribution
    result.rgb *= weight[0];
    if(horizontal)
    {
        for(int i = 1; i < 5; ++i)
        {
            result.rgb += texture(image, TexCoords + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
            result.rgb += texture(image, TexCoords - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
        }
    }
    else
    {
        for(int i = 1; i < 5; ++i)
        {
            result.rgb += texture(image, TexCoords + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
            result.rgb += texture(image, TexCoords - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
        }
    }

    FragColor = vec4(result.rgb, result.a);  */

}