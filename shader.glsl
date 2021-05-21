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
layout (location = 2) out vec3 gPos;
layout (location = 3) out uint gVoxelID;

struct OctreeNode {
    uint parentIndex;
    uint childrenIndices[8];
    int isSolidColor;
    uint dataIndex;
};

struct gBufferData {
    vec3 albedo;
    vec3 normal;
    vec3 pos;
    uint voxelID;
};

layout(std430, binding = 0) buffer OctreeSSBO {
    OctreeNode octreeNodes[];
};

layout(std430, binding = 1) buffer ChunkDataSSBO {
    uint chunkData[];
};

uniform vec3 u_cameraPos;
uniform mat3 u_cameraRotMatrix;
uniform uint u_worldWidth;
uniform uint u_maxOctreeDepth;
uniform uint u_chunkWidth;
uniform vec3 u_palette[256];
uniform ivec2 u_windowSize;
uniform float u_fov;

in vec2 fragPos;

uint chunkWidthSquared = u_chunkWidth * u_chunkWidth;

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

// Calculates the center of the next voxel and the normal by traversing a ray starting on 'cameraPos' with direction 'rayDir'. 
vec3 getNextVoxel(vec3 cubeCenterPos, inout vec3 normal, inout float rayLength, vec3 cameraPos, float cubeWidth, vec3 rayDir, vec3 invRayDir) {
    // cameraPos + rayDir * dRay = cubeCenterPos +- width/2 <=> dRay = (cubeCenterPos +- width/2 - cameraPos) / rayDir
    vec3 dPos = cubeCenterPos + vec3(((rayDir.x >= 0) ? cubeWidth : -cubeWidth), ((rayDir.y >= 0) ? cubeWidth : -cubeWidth), ((rayDir.z >= 0) ? cubeWidth : -cubeWidth)) * 0.5 - cameraPos;
    vec3 dRay = dPos * invRayDir;

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
    cubeCenterPos = cameraPos + rayLength * rayDir - normal * 0.5;

    return floor(cubeCenterPos) + vec3(0.5, 0.5, 0.5);
}

// Raymarches through a chunk and returns the paletteIndex of the first voxel hit, or zero if no voxels were hit. If a voxel was hit its local position, specified
//  in chunk space, and the normal where the ray hit the voxel are returned in the arguments 'localVoxelPos' and 'normal'.
//  localVoxelPos should always be the position of the center of a voxel.
uint getVoxelData(uint chunkDataIndex, inout vec3 localVoxelPos, inout vec3 normal, inout float rayLength, vec3 localCameraPos, vec3 rayDir, vec3 invRayDir) {
    for(int iteration = 0; iteration < 32; ++iteration) {
        if(localVoxelPos.x < 0 || localVoxelPos.x >= u_chunkWidth || localVoxelPos.y < 0.0 || localVoxelPos.y >= u_chunkWidth || localVoxelPos.z < 0.0 || localVoxelPos.z >= u_chunkWidth) {
            break;
        }

        uint voxelByte = getVoxelByte(chunkDataIndex, ivec3(floor(localVoxelPos)));
        if(voxelByte != 0) {
            return voxelByte;
        }

        localVoxelPos = getNextVoxel(localVoxelPos, normal, rayLength, localCameraPos, 1.0, rayDir, invRayDir);        
    }

    return 0;
}

// Calculates the gBuffer data by raymarching through an octree 
gBufferData getGBufferData(vec3 pos, vec3 rayDir, uint maxIterations) {
    gBufferData result;
    
    vec3 cameraPos = pos;
    vec3 voxelPos = floor(pos) + vec3(0.5, 0.5, 0.5); // voxelPos is always in the center of a voxel
    vec3 normal = vec3(1.0, 0.0, 0.0);
    float rayLength = 0;

    vec3 invRayDir = 1.0 / rayDir;
    float hWorldWidth = u_worldWidth / 2.0;

    int iteration;
    for(iteration = 0; iteration < maxIterations; ++iteration) {
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
                uint voxelPaletteIndex = getVoxelData(octreeNodes[currentOctreeNodeID].dataIndex, localVoxelPos, normal, rayLength, cameraPos + (localVoxelPos - voxelPos), rayDir, invRayDir);
                if(voxelPaletteIndex != 0) {
                    result.albedo = u_palette[voxelPaletteIndex];
                    result.normal = normal;
                    result.pos = cameraPos + rayLength * rayDir;
                    result.voxelID = voxelPaletteIndex;
                    return result;
                }
            }
        }
        else if(octreeNodes[currentOctreeNodeID].dataIndex != 0) { // Every voxel in the current octree node is the same color
            float octreeNodeWidth = u_worldWidth / pow(2, currentDepth);
            vec3 localChunkPos = vec3(localOctreeNodeVoxelPos) + vec3(octreeNodeWidth) * 0.5;

            result.albedo = u_palette[octreeNodes[currentOctreeNodeID].dataIndex];
            result.pos = cameraPos + rayLength * rayDir;
            result.normal = normal;
            result.voxelID = octreeNodes[currentOctreeNodeID].dataIndex;
            return result;
        }

        float width = u_worldWidth / pow(2, currentDepth);
        vec3 octreeNodePos = floor(voxelPos / width) * width + vec3(width * 0.5);  // position of the center of the current octreeNode

        voxelPos = getNextVoxel(octreeNodePos, normal, rayLength, cameraPos, width, rayDir, invRayDir);
    }

    result.albedo = vec3(-1.0, -1.0, -1.0);
    result.normal = vec3(0.0, 0.0, 0.0);
    result.pos = vec3(0.0, 0.0, 0.0);
    result.voxelID = 0;
    return result;
}

vec3 getCameraRayDir(vec2 screenSpaceCoordinates, mat3 cameraRotMatrix, float aspectRatio, float fov) {
    vec3 rayDirCamera;
    rayDirCamera.x = screenSpaceCoordinates.x * tan(fov) * aspectRatio;
    rayDirCamera.y = screenSpaceCoordinates.y * tan(fov);
    rayDirCamera.z = -1.0;
    rayDirCamera = normalize(rayDirCamera);

    return cameraRotMatrix * rayDirCamera;
}

void main() {
    vec3 pos = vec3(u_cameraPos);

    float aspectRatio = u_windowSize.x / float(u_windowSize.y);
    vec2 screenSpaceCoordinates = fragPos - vec2(0.5, 0.5);

    vec3 rayDir = getCameraRayDir(screenSpaceCoordinates, u_cameraRotMatrix, aspectRatio, u_fov);

    gBufferData gbd = getGBufferData(pos, rayDir, 100);
    gAlbedo = gbd.albedo;
    gNormal = gbd.normal;
    gPos = gbd.pos;
    gVoxelID = gbd.voxelID;
}