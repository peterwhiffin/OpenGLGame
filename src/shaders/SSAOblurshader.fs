#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

layout (binding = 0) uniform sampler2D tex;

void main() 
{
    vec2 texelSize = 1.0 / vec2(textureSize(tex, 0));
    vec4 result = vec4(0.0);
    for (int x = -2; x < 2; ++x) 
    {
        for (int y = -2; y < 2; ++y) 
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(tex, TexCoords + offset);
        }
    }
    
    result = result / (4.0 * 4.0);
    FragColor = result;
}   