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

layout (location = 0) out vec3 gAlbedo;
layout (location = 1) out vec3 gNormal;

struct OctreeNode {
    uint parentIndex;
    uint childrenIndices[8];
    int isSolidColor;
    uint dataIndex;
};

struct VoxelData {
    uint paletteIndex;
    vec3 hitPos;
    float rayLength;
};

struct gBufferData {
    vec3 albedo;
    vec3 normal;
};

layout(std430, binding = 0) buffer OctreeSSBO {
    OctreeNode octreeNodes[];
};

layout(std430, binding = 1) buffer ChunkDataSSBO {
    uint chunkData[];
};

uniform vec3 u_cameraPos;
uniform float u_cameraDir;
uniform uint u_worldWidth;
uniform uint u_maxOctreeDepth;
uniform uint u_chunkWidth;
uniform vec3 u_palette[256];
uniform ivec2 u_windowSize;
uniform float u_fov;

in vec2 fragPos;

const float deltaRayOffset = 0.01;
uint chunkWidthSquared = u_chunkWidth * u_chunkWidth;

vec3 getNormal(vec3 localHitPos) {
    float thLow = deltaRayOffset;
    float thHigh = 1.0 - thLow;
    float nx = ((localHitPos.x < thLow) ? -1.0 : 0.0) + ((localHitPos.x > thHigh) ? 1.0 : 0.0);
    float ny = ((localHitPos.y < thLow) ? -1.0 : 0.0) + ((localHitPos.y > thHigh) ? 1.0 : 0.0);
    float nz = ((localHitPos.z < thLow) ? -1.0 : 0.0) + ((localHitPos.z > thHigh) ? 1.0 : 0.0);

    return vec3(nx, ny, nz);
}

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

VoxelData getVoxelData(uint chunkDataIndex, vec3 localPos, vec3 rayDir, vec3 invRayDir) {
    VoxelData voxelData;
    voxelData.paletteIndex = 0;
    voxelData.rayLength = 0;

    ivec3 iLocalPos;
    for(int iteration = 0; iteration < 100; ++iteration) {
        iLocalPos = ivec3(floor(localPos.x), floor(localPos.y), floor(localPos.z));
        if(iLocalPos.x < 0 || iLocalPos.x >= u_chunkWidth || iLocalPos.y < 0.0 || iLocalPos.y >= u_chunkWidth || iLocalPos.z < 0.0 || iLocalPos.z >= u_chunkWidth) {
            break;
        }

        uint voxelByte = getVoxelByte(chunkDataIndex, iLocalPos);
        if(voxelByte != 0) {
            voxelData.paletteIndex = voxelByte;
            voxelData.hitPos = localPos - iLocalPos;
            break;
        }

        float deltaRay = getDeltaRay(localPos, iLocalPos, 1.0, rayDir, invRayDir) + deltaRayOffset;
        localPos += rayDir * deltaRay;
        voxelData.rayLength += deltaRay;
    }

    return voxelData;
}

gBufferData getGBufferData(vec3 pos, vec3 rayDir, uint maxIterations) {
    gBufferData result;
    
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
                VoxelData voxelData = getVoxelData(octreeNodes[currentOctreeNodeID].dataIndex, localChunkPos, rayDir, invRayDir);
                if(voxelData.paletteIndex != 0) {
                    rayLength += voxelData.rayLength;

                    result.albedo = u_palette[voxelData.paletteIndex];
                    result.normal = getNormal(voxelData.hitPos);
                    return result;
                }
            }
        }
        else if(octreeNodes[currentOctreeNodeID].dataIndex != 0) { // Every voxel in the current octree node is the same color
            float octreeNodeWidth = u_worldWidth / pow(2, currentDepth);
            vec3 localChunkPos = localPos + vec3(octreeNodeWidth) * 0.5;

            result.albedo = u_palette[octreeNodes[currentOctreeNodeID].dataIndex];
            result.normal = getNormal(localChunkPos / octreeNodeWidth);
            return result;
        }

        float width = u_worldWidth / pow(2, currentDepth);
        ivec3 ipos = ivec3(floor(pos / width) * width);
        float deltaRay = getDeltaRay(pos, ipos, width, rayDir, invRayDir) + deltaRayOffset;
        pos += rayDir * deltaRay;

        rayLength += deltaRay;

    }

    result.albedo = vec3(0.0, 0.0, 0.0);
    result.normal = vec3(0.0, 0.0, 0.0);
    return result;
}

void main() {
    vec3 pos = vec3(u_cameraPos);

    float aspectRatio = u_windowSize.x / float(u_windowSize.y);
    vec2 screenSpaceCoordinates = fragPos - vec2(0.5, 0.5);

    vec3 rayDirCamera;
    rayDirCamera.x = screenSpaceCoordinates.x * tan(u_fov) * aspectRatio;
    rayDirCamera.y = screenSpaceCoordinates.y * tan(u_fov);
    rayDirCamera.z = -1.0;
    rayDirCamera = normalize(rayDirCamera);

    vec3 rayDir;
    rayDir.x = rayDirCamera.x * cos(u_cameraDir) - rayDirCamera.z * sin(u_cameraDir);
    rayDir.y = rayDirCamera.y;
    rayDir.z = rayDirCamera.x * sin(u_cameraDir) + rayDirCamera.z * cos(u_cameraDir);
    rayDir = normalize(rayDir);

    gBufferData gbd = getGBufferData(pos, rayDir, 100);
    gAlbedo = gbd.albedo;
    gNormal = gbd.normal;
}