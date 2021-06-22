#section vertex
#version 430 core
layout (location = 0) in vec3 aPos;

out vec2 fragPos;

void main() {
    fragPos = vec2((aPos.x + 1.0) / 2.0, (aPos.y + 1.0) / 2.0);
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}


#section fragment
#version 430 core

out vec4 FragColor;
in vec2 fragPos;

uniform sampler2D u_frameTexture;
uniform sampler2D u_albedoTexture;
uniform sampler2D u_normalTexture;
uniform sampler2D u_posTexture;

uniform vec2 u_windowSize;
uniform float u_colorWeightScaler;
uniform float u_normalWeightScaler;
uniform float u_posWeightScaler;
uniform float u_scale;

float kernel[3][3] = {
    {1, 2, 1},
    {2, 4, 2},
    {1, 2, 1}
};

void main() {
    vec2 pixelSize = 1.0 / u_windowSize;

    vec3 c0 = texture(u_frameTexture, fragPos).rgb;
    vec3 albedo = texture(u_albedoTexture, fragPos).rgb;
    vec3 normal = texture(u_normalTexture, fragPos).xyz;
    vec3 pos = texture(u_posTexture, fragPos).xyz;

    float colorWeightScaler = u_colorWeightScaler;
    float normalWeightScaler = u_normalWeightScaler;
    float posWeightScaler = u_posWeightScaler;

    vec3 c1 = vec3(0.0);
    float k = 0.0;
    for(int dy = 0; dy < 3; ++dy) {
        for(int dx = 0; dx < 3; ++dx) {
            vec2 screenPos = fragPos + vec2(dx - 1.0, dy - 1.0) * pixelSize * u_scale;

            float h = float(kernel[dy][dx]);

            vec3 otherAlbedo = texture(u_albedoTexture, screenPos).rgb;
            vec3 albedoDiff = albedo - otherAlbedo;
            float albedoDistSqr = dot(albedoDiff, albedoDiff);
            float weightAlbedo = min(exp(-albedoDistSqr / colorWeightScaler), 1.0);

            vec3 otherNormal = texture(u_normalTexture, screenPos).xyz;
            vec3 normalDiff = normal - otherNormal;
            float normalDistSqr = dot(normalDiff, normalDiff);
            float weightNormal = min(exp(-normalDistSqr / normalWeightScaler), 1.0);

            vec3 otherPos = texture(u_posTexture, screenPos).xyz;
            vec3 posDiff = pos - otherPos;
            float posDistSqr = dot(posDiff, posDiff);
            float weightPos = min(exp(-posDistSqr / posWeightScaler), 1.0);

            float weight = weightAlbedo * weightNormal * weightPos;

            k += h * weight;

            c1 += h * weight * texture(u_frameTexture, screenPos).rgb;
        }
    }
    c1 /= k;

    FragColor = vec4(c1, 1.0);
}
