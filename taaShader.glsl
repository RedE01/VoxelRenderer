#section vertex
#version 430 core
layout (location = 0) in vec3 aPos;

void main() {
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}


#section fragment
#version 430 core

layout (location = 0) out vec4 frameTexture;

uniform sampler2D u_frameTexture;
uniform sampler2D u_normalTexture;
uniform sampler2D u_posTexture;
uniform sampler2D u_prevFrameTexture;
uniform sampler2D u_prevNormalTexture;
uniform sampler2D u_prevPosTexture;

uniform vec3 u_cameraPos;
uniform mat3 u_prevCameraRotMatrix;
uniform vec3 u_prevCameraPos;
uniform vec2 u_windowSize;
uniform float u_fov;
uniform float u_taaAlpha;
uniform float u_taaDistWeightScaler;
uniform float u_taaNormalWeightScaler;
uniform float u_taaColorWeightScaler;

vec2 getScreenSpacePosition(vec3 worldSpacePos, vec3 cameraPos, mat3 cameraRotMatrix, float aspectRatio, float fov) {
    vec3 rayDir = normalize(worldSpacePos - cameraPos); // Ray dir in world space
    vec3 rayDirCamera = transpose(cameraRotMatrix) * rayDir; // Ray dir in camera space
    float scale = -1.0 / rayDirCamera.z;
    rayDirCamera *= scale;

    vec2 screenSpaceCoordinates;
    screenSpaceCoordinates.x = rayDirCamera.x / (tan(fov) * aspectRatio);
    screenSpaceCoordinates.y = rayDirCamera.y / tan(fov);
    screenSpaceCoordinates += vec2(0.5);
    return screenSpaceCoordinates;
}

void main() {
    vec2 fragPos = gl_FragCoord.xy / u_windowSize;
    vec3 framePixel = texture(u_frameTexture, fragPos).rgb;
    vec3 normal = texture(u_normalTexture, fragPos).xyz;
    vec3 pos = texture(u_posTexture, fragPos).xyz;

    float aspectRatio = u_windowSize.x / u_windowSize.y;
    vec2 screenSpaceCoordinates = getScreenSpacePosition(pos, u_prevCameraPos, u_prevCameraRotMatrix, aspectRatio, u_fov);

    vec2 pixelSize = 1.0 / u_windowSize;
    float n = 0.0;
    vec3 accumulatedPixel = vec3(0.0);

    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            vec2 pixelPos = screenSpaceCoordinates + vec2(x * pixelSize.x, y * pixelSize.y);
            if(pixelPos.x >= 0.0 && pixelPos.x <= 1.0 && pixelPos.y >= 0.0 && pixelPos.y <= 1.0) {
                vec3 prevPixel = texture(u_prevFrameTexture, pixelPos).rgb;
                vec3 prevNormal = texture(u_prevNormalTexture, pixelPos).rgb;
                vec3 prevFragmentPos = texture(u_prevPosTexture, pixelPos).xyz;
                
                vec3 deltaFragmentPos = pos - prevFragmentPos;
                float dist = distance(pos, prevFragmentPos);
                float distWeight = min(exp(-dist * u_taaDistWeightScaler), 1.0);

                float normalDiff = dot(normal, prevNormal) - 1.0;
                float normalWeight = min(exp((normalDiff * u_taaNormalWeightScaler)), 1.0);

                float colorDiff = distance(framePixel, prevPixel);
                float colorWeight = min(exp(colorDiff * u_taaColorWeightScaler), 1.0);

                float weight = distWeight * normalWeight * colorWeight;

                accumulatedPixel += prevPixel * weight;
                n += weight;
            }
        }
    }

    vec3 result;
    if(n > 0) result = accumulatedPixel / n;
    else result = framePixel;

    frameTexture = vec4((1.0 - u_taaAlpha) * result + u_taaAlpha * framePixel, 1.0);
}