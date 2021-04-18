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

uniform sampler2D gAlbedo;
uniform sampler2D gNormal;
uniform sampler2D gPos;

uniform uint u_worldWidth;
uniform uint u_maxOctreeDepth;
uniform uint u_chunkWidth;

in vec2 fragPos;

uniform float u_deltaTime;

const float deltaRayOffset = 0.01;
uint chunkWidthSquared = u_chunkWidth * u_chunkWidth;

uint getVoxelByte(uint chunkDataIndex, ivec3 iLocalPos) {
    uint localVoxelID = iLocalPos.x + iLocalPos.y * u_chunkWidth + iLocalPos.z * chunkWidthSquared;
    uint voxelID = chunkDataIndex + localVoxelID;

    // Each voxel is one byte but we index it as a uint, so the voxelID is divided by 4 and the appropriate byte is returned.
    uint voxelIndex = voxelID >> 2;
    uint voxelDataWord = chunkData[voxelIndex];
    return (voxelDataWord >> ((voxelID % 4) << 3)) & uint(0x000000FF);
}

float getDeltaRay(vec3 pos, ivec3 ipos, float cubeWidth, vec3 rayDir, vec3 invRayDir) {
    vec3 dPos = vec3(((rayDir.x > 0) ? 1 : 0), ((rayDir.y > 0) ? 1 : 0), ((rayDir.z > 0) ? 1 : 0)) * cubeWidth + ipos - pos;

    vec3 dRay = dPos * invRayDir;

    return min(dRay.x, min(dRay.y, dRay.z));
}

void getOctreeNode(inout uint currentOctreeNodeID, inout uint depth, inout vec3 pos) {
    while(octreeNodes[currentOctreeNodeID].isSolidColor == 0 && depth < u_maxOctreeDepth) {
        int childIndex = ((pos.x >= 0) ? 1 : 0) + ((pos.y >= 0) ? 1 : 0) * 2 + ((pos.z >= 0) ? 1 : 0) * 4;

        float qWidth = u_worldWidth / pow(2, depth + 2);
        pos.x += qWidth * ((pos.x >= 0) ? -1.0 : 1.0);
        pos.y += qWidth * ((pos.y >= 0) ? -1.0 : 1.0);
        pos.z += qWidth * ((pos.z >= 0) ? -1.0 : 1.0);

        depth++;
        currentOctreeNodeID = octreeNodes[currentOctreeNodeID].childrenIndices[childIndex];
    }
}

float getRayLengthInChunk(uint chunkDataIndex, vec3 localPos, vec3 rayDir, vec3 invRayDir) {
    float rayLength = 0.0;
    ivec3 iLocalPos;
    for(int iteration = 0; iteration < 100; ++iteration) {
        iLocalPos = ivec3(floor(localPos.x), floor(localPos.y), floor(localPos.z));
        if(iLocalPos.x < 0 || iLocalPos.x >= u_chunkWidth || iLocalPos.y < 0.0 || iLocalPos.y >= u_chunkWidth || iLocalPos.z < 0.0 || iLocalPos.z >= u_chunkWidth) {
            break;
        }

        uint voxelByte = getVoxelByte(chunkDataIndex, iLocalPos);
        if(voxelByte != 0) {
            return rayLength;
        }

        float deltaRay = getDeltaRay(localPos, iLocalPos, 1.0, rayDir, invRayDir) + deltaRayOffset;
        localPos += rayDir * deltaRay;
        rayLength += deltaRay;
    }

    return -1.0;
}

float getRayLength(vec3 pos, vec3 rayDir, uint maxIterations) {
    vec3 cameraPos = pos;
    float rayLength = 0.0;

    vec3 invRayDir = 1.0 / rayDir;
    float hWorldWidth = u_worldWidth / 2.0;

    int iteration;
    for(iteration = 0; iteration < maxIterations; ++iteration) {
        if(pos.x <= -hWorldWidth || pos.x >= hWorldWidth || pos.y <= -hWorldWidth || pos.y >= hWorldWidth || pos.z <= -hWorldWidth || pos.z >= hWorldWidth) {
            break;
        }

        uint currentOctreeNodeID = 0;
        uint currentDepth = 0;
        vec3 localPos = pos;
        getOctreeNode(currentOctreeNodeID, currentDepth, localPos);

        if(octreeNodes[currentOctreeNodeID].isSolidColor == 0) {
            if(currentDepth == u_maxOctreeDepth) { // Search for voxel in current chunk
                vec3 localChunkPos = localPos + vec3(u_chunkWidth) * 0.5;
                float chunkRayLength = getRayLengthInChunk(octreeNodes[currentOctreeNodeID].dataIndex, localChunkPos, rayDir, invRayDir);
                if(chunkRayLength >= 0.0) {
                    rayLength += chunkRayLength;

                    return rayLength;
                }
            }
        }
        else if(octreeNodes[currentOctreeNodeID].dataIndex != 0) { // Every voxel in the current octree node is the same color
            return rayLength;
        }

        float width = u_worldWidth / pow(2, currentDepth);
        ivec3 ipos = ivec3(floor(pos / width) * width);
        float deltaRay = getDeltaRay(pos, ipos, width, rayDir, invRayDir) + deltaRayOffset;
        pos += rayDir * deltaRay;

        rayLength += deltaRay;

    }

    return rayLength;
}


float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);
}

vec3 getRayDir(vec3 normal, vec2 screenPos, float deltaTime) {
    float rand1 = random(screenPos + vec2(deltaTime, 0.0)) * 2.0 - 1.0;
    float rand2 = random(screenPos + vec2(0.0, deltaTime)) * 2.0 - 1.0;

    vec3 rayDir;
    if(normal.x != 0.0) rayDir = vec3(normal.x, rand1, rand2);
    else if(normal.y != 0.0) rayDir = vec3(rand1, normal.y, rand2);
    else rayDir = vec3(rand1, rand2, normal.z);
    
    rayDir = normalize(rayDir);

    return rayDir;
}

void main() {
    vec3 albedo = texture(gAlbedo, fragPos).rgb;
    vec3 normal = texture(gNormal, fragPos).xyz;
    vec3 pos = texture(gPos, fragPos).xyz;

    uint iterations = (albedo == vec3(-1, -1, -1)) ? 0 : 8;
    vec3 rayDir = getRayDir(normal, fragPos, u_deltaTime);
    float rayLength = getRayLength(pos + rayDir * 0.1, rayDir, iterations);
    float oclusion = min(pow(rayLength, 1.0), 1.0);

    FragColor = vec4(vec3(oclusion) * albedo, 1.0);

}