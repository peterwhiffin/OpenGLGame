/* #version 460 core
out float FragColor;

in vec2 TexCoords;

layout (binding = 0) uniform sampler2D tex;

void main() 
{
    vec2 texelSize = 1.0 / vec2(textureSize(tex, 0));
    float result = 0.0;
    for (int x = -2; x < 2; ++x) 
    {
        for (int y = -2; y < 2; ++y) 
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(tex, TexCoords + offset).r;
        }
    }
    
    result = result / (4.0 * 4.0);
    FragColor = result;
} */  

#version 460 core
out float FragColor;

in vec2 TexCoords;

layout (binding = 0) uniform sampler2D tex;

const float kernel[5][5] = float[5][5](
    float[](1, 4, 7, 4, 1),
    float[](4, 16, 26, 16, 4),
    float[](7, 26, 41, 26, 7),
    float[](4, 16, 26, 16, 4),
    float[](1, 4, 7, 4, 1)
);

void main()
{
    vec2 texelSize = 1.0 / vec2(textureSize(tex, 0));
    float centerDepth = texture(tex, TexCoords).r;
    
    float sigma = 1.1;  // Controls depth sensitivity
    float result = 0.0;
    float weightSum = 0.0;
    
    for (int x = -2; x <= 2; ++x)
    {
        for (int y = -2; y <= 2; ++y)
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            float sampleDepth = texture(tex, TexCoords + offset).r;
            
            float depthDiff = centerDepth - sampleDepth;
            float rangeWeight = exp(-(depthDiff * depthDiff) / (2.0 * sigma * sigma));
            
            float spatialWeight = kernel[x + 2][y + 2];
            
            float weight = spatialWeight * rangeWeight;
            
            result += sampleDepth * weight;
            weightSum += weight;
        }
    }
    
    result /= weightSum;
    FragColor = result;
}