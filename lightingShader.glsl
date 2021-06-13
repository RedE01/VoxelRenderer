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

layout (location = 0) out vec4 frameTexture;

struct OctreeNode {
    uint parentIndex;
    uint childrenIndices[8];
    int isSolidColor;
    uint dataIndex;
};

layout(std430, binding = 0) buffer OctreeSSBO {
    OctreeNode octreeNodes[];
};

layout(std430, binding = 1) buffer ChunkDataSSBO {
    uint chunkData[];
};

uniform sampler2D u_gAlbedo;
uniform sampler2D u_gNormal;
uniform sampler2D u_gPos;
uniform sampler2D u_blueNoiseTexture;

uniform uint u_worldWidth;
uniform uint u_maxOctreeDepth;
uniform uint u_chunkWidth;

uniform vec2 u_noiseTextureScale;
uniform float u_frame;

in vec2 fragPos;

uint chunkWidthSquared = u_chunkWidth * u_chunkWidth;
float phi1 = 1.6180339887498948; // x^2 = x + 1
float phi2 = 1.3247179572447460; // x^3 = x + 1

uint getVoxelByte(uint chunkDataIndex, ivec3 iLocalPos) {
    uint localVoxelID = iLocalPos.x + iLocalPos.y * u_chunkWidth + iLocalPos.z * chunkWidthSquared;
    uint voxelID = chunkDataIndex + localVoxelID;

    // Each voxel is one byte but we index it as a uint, so the voxelID is divided by 4 and the appropriate byte is returned.
    uint voxelIndex = voxelID >> 2;
    uint voxelDataWord = chunkData[voxelIndex];
    return (voxelDataWord >> ((voxelID % 4) << 3)) & uint(0x000000FF);
}

// Calculates the octreeID of the octreeNode containing the given position.
//  'currentOctreeNodeID' must be the id of a parent of the wanted node (should most of the time be 0). The wanted node id is returned in this variable.
//  'depth' must the depth of the node provided (should also most of the time be 0). The wanted nodes depth in the octree is returned in this variable.
//  'pos' must be local to the provided node (0,0) being the center of the parent node (if currentOctreeNodeID is zero this is in global coordinates).
//      The provided position in local space of the calculated octreeNode is returned in this variable.
void getOctreeNode(inout uint currentOctreeNodeID, inout uint depth, inout vec3 pos) {
    while(octreeNodes[currentOctreeNodeID].isSolidColor == 0 && depth < u_maxOctreeDepth) {
        int childIndex = ((pos.x >= 0) ? 1 : 0) + ((pos.y >= 0) ? 1 : 0) * 2 + ((pos.z >= 0) ? 1 : 0) * 4;

        float qWidth = u_worldWidth / pow(2, depth + 2);
        pos.x += qWidth * ((pos.x >= 0) ? -1 : 1);
        pos.y += qWidth * ((pos.y >= 0) ? -1 : 1);
        pos.z += qWidth * ((pos.z >= 0) ? -1 : 1);

        depth++;
        currentOctreeNodeID = octreeNodes[currentOctreeNodeID].childrenIndices[childIndex];
    }
}

// Calculates the center of the next voxel by traversing a ray starting on 'startPos' with direction 'rayDir'. 
vec3 getNextVoxel(vec3 cubeCenterPos, inout float rayLength, vec3 startPos, float cubeWidth, vec3 rayDir, vec3 invRayDir) {
    // startPos + rayDir * dRay = cubeCenterPos +- width/2 <=> dRay = (cubeCenterPos +- width/2 - startPos) / rayDir
    vec3 dPos = cubeCenterPos + vec3(((rayDir.x >= 0) ? cubeWidth : -cubeWidth), ((rayDir.y >= 0) ? cubeWidth : -cubeWidth), ((rayDir.z >= 0) ? cubeWidth : -cubeWidth)) * 0.5 - startPos;
    vec3 dRay = dPos * invRayDir;

    vec3 normal;
    if(dRay.x < dRay.y && dRay.x < dRay.z) {
        normal = vec3(-sign(rayDir.x), 0.0, 0.0);
        rayLength = dRay.x;
    }
    else if(dRay.y < dRay.z) {
        normal = vec3(0.0, -sign(rayDir.y), 0.0);
        rayLength = dRay.y;
    }
    else {
        normal = vec3(0.0, 0.0, -sign(rayDir.z));
        rayLength = dRay.z;
    }
    cubeCenterPos = startPos + rayLength * rayDir - normal * 0.5;

    return floor(cubeCenterPos) + vec3(0.5, 0.5, 0.5);
}

// Raymarches through a chunk and returns the length of the ray untill the first voxel is hit, or -1 if no voxels were hit.
//  localVoxelPos should always be the position of the center of a voxel.
float getRayLengthInChunk(uint chunkDataIndex, vec3 localVoxelPos, vec3 localStartPos, vec3 rayDir, vec3 invRayDir) {
    float rayLength = 0.0;
    for(int iteration = 0; iteration < 16; ++iteration) {
        if(localVoxelPos.x < 0 || localVoxelPos.x >= u_chunkWidth || localVoxelPos.y < 0.0 || localVoxelPos.y >= u_chunkWidth || localVoxelPos.z < 0.0 || localVoxelPos.z >= u_chunkWidth) {
            break;
        }

        uint voxelByte = getVoxelByte(chunkDataIndex, ivec3(floor(localVoxelPos)));
        if(voxelByte != 0) {
            return rayLength;
        }

        localVoxelPos = getNextVoxel(localVoxelPos, rayLength, localStartPos, 1.0, rayDir, invRayDir);        
    }

    return -1;
}

// Calculates the length of a ray untill it reaches a voxel by raymarching through an octree 
float getRayLength(vec3 pos, vec3 rayDir, uint maxIterations, float maxDistance) {
    vec3 startPos = pos;
    vec3 voxelPos = floor(pos) + vec3(0.5, 0.5, 0.5); // voxelPos is always in the center of a voxel
    vec3 normal = vec3(1.0, 0.0, 0.0);
    float rayLength = 0;

    vec3 invRayDir = 1.0 / rayDir;
    float hWorldWidth = u_worldWidth / 2.0;

    for(uint iterations = 0; iterations < maxIterations && rayLength < maxDistance; ++iterations) {
        if(voxelPos.x <= -hWorldWidth || voxelPos.x >= hWorldWidth || voxelPos.y <= -hWorldWidth || voxelPos.y >= hWorldWidth || voxelPos.z <= -hWorldWidth || voxelPos.z >= hWorldWidth) {
            break;
        }

        uint currentOctreeNodeID = 0;
        uint currentDepth = 0;
        vec3 localOctreeNodeVoxelPos = voxelPos;
        getOctreeNode(currentOctreeNodeID, currentDepth, localOctreeNodeVoxelPos);

        if(octreeNodes[currentOctreeNodeID].isSolidColor == 0) {
            if(currentDepth == u_maxOctreeDepth) { // Search for voxel in current chunk
                // localOctreeNodeVoxelPos is in the range [-width/2, width/2], we want to transform it into the range [0, width]
                vec3 localVoxelPos = floor(localOctreeNodeVoxelPos + vec3(u_chunkWidth * 0.5)) + vec3(0.5);
                rayLength = getRayLengthInChunk(octreeNodes[currentOctreeNodeID].dataIndex, localVoxelPos, startPos + (localVoxelPos - voxelPos), rayDir, invRayDir);
                if(rayLength >= 0.0) {
                    return rayLength;
                }
            }
        }
        else if(octreeNodes[currentOctreeNodeID].dataIndex != 0) { // Every voxel in the current octree node is the same color
            float octreeNodeWidth = u_worldWidth / pow(2, currentDepth);
            vec3 localChunkPos = vec3(localOctreeNodeVoxelPos) + vec3(octreeNodeWidth) * 0.5;

            return rayLength;
        }

        float width = u_worldWidth / pow(2, currentDepth);
        vec3 octreeNodePos = floor(voxelPos / width) * width + vec3(width * 0.5);  // position of the center of the current octreeNode

        voxelPos = getNextVoxel(octreeNodePos, rayLength, startPos, width, rayDir, invRayDir);
    }

    return maxDistance;
}

float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);
}

vec3 getRandomRayDir(vec3 normal, vec2 noiseSamplePos, float frame) {
    vec2 rand = texture(u_blueNoiseTexture, noiseSamplePos).xy;
    rand.x = mod(rand.x + phi1 * mod(frame, 128.0), 1.0) * 2.0 - 1.0;
    rand.y = mod(rand.y + phi2 * mod(frame, 128.0), 1.0) * 2.0 - 1.0;
    rand *= 2.0;

    vec3 rayDir;
    if(normal.x != 0.0) rayDir = vec3(normal.x, rand.x, rand.y);
    else if(normal.y != 0.0) rayDir = vec3(rand.x, normal.y, rand.y);
    else rayDir = vec3(rand.x, rand.y, normal.z);
    
    rayDir = normalize(rayDir);

    return rayDir;
}

vec2 getScreenSpacePosition(vec3 worldSpacePos, vec3 cameraPos, mat3 cameraRotMatrix, float aspectRatio, float fov) {
    vec3 rayDir = normalize(worldSpacePos - cameraPos); // Ray dir in world space
    vec3 rayDirCamera = transpose(cameraRotMatrix) * rayDir; // Ray dir in camera space
    rayDirCamera = normalize(rayDirCamera);
    float scale = -1.0 / rayDirCamera.z;
    rayDirCamera *= scale;

    vec2 screenSpaceCoordinates;
    screenSpaceCoordinates.x = rayDirCamera.x / (tan(fov) * aspectRatio);
    screenSpaceCoordinates.y = rayDirCamera.y / tan(fov);
    screenSpaceCoordinates += vec2(0.5);
    return screenSpaceCoordinates;
}

void main() {
    vec3 albedo = texture(u_gAlbedo, fragPos).rgb;
    vec3 normal = texture(u_gNormal, fragPos).xyz;
    vec3 pos = texture(u_gPos, fragPos).xyz;

    uint maxIterations = (albedo.x < 0.0) ? 0 : 16;
    float maxDistance = 16.0;
    vec3 rayDir = getRandomRayDir(normal, fragPos * u_noiseTextureScale, u_frame);
    float rayLength = getRayLength(pos + rayDir * 0.01, rayDir, maxIterations, maxDistance);
    float oclusion = rayLength / maxDistance;
    oclusion = min(pow(oclusion, 0.8), 1.0);

    vec3 currentPixelValue = albedo * oclusion;
    frameTexture = vec4(currentPixelValue, 1.0);
}