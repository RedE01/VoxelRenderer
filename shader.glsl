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

const double deltaRayOffset = 0.000000001;

struct OctreeNode {
    uint parentIndex;
    uint childrenIndices[8];
    int isSolidColor;
    uint dataIndex;
};

struct VoxelData {
    uint paletteIndex;
    dvec3 hitPos;
    double rayLength;
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

out vec4 FragColor;
in vec2 fragPos;

uint chunkWidthSquared = u_chunkWidth * u_chunkWidth;

vec3 getNormal(VoxelData voxelData) {
    dvec3 localHitPos = voxelData.hitPos;

    double thLow = deltaRayOffset;
    double thHigh = 1.0 - thLow;
    double nx = ((localHitPos.x < thLow) ? -1.0 : 0.0) + ((localHitPos.x > thHigh) ? 1.0 : 0.0);
    double ny = ((localHitPos.y < thLow) ? -1.0 : 0.0) + ((localHitPos.y > thHigh) ? 1.0 : 0.0);
    double nz = ((localHitPos.z < thLow) ? -1.0 : 0.0) + ((localHitPos.z > thHigh) ? 1.0 : 0.0);

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

double getDeltaRay(dvec3 pos, ivec3 ipos, double cubeWidth, dvec3 rayDir, dvec3 invRayDir) {
    dvec3 dPos = dvec3(((rayDir.x > 0) ? 1 : 0), ((rayDir.y > 0) ? 1 : 0), ((rayDir.z > 0) ? 1 : 0)) * cubeWidth + ipos - pos;

    dvec3 dRay = dPos * invRayDir;

    return min(dRay.x, min(dRay.y, dRay.z));
}

void getOctreeNode(inout uint currentOctreeNodeID, inout uint depth, inout dvec3 pos) {
    while(octreeNodes[currentOctreeNodeID].isSolidColor == 0 && depth < u_maxOctreeDepth) {
        int childIndex = ((pos.x >= 0) ? 1 : 0) + ((pos.y >= 0) ? 1 : 0) * 2 + ((pos.z >= 0) ? 1 : 0) * 4;

        double qWidth = u_worldWidth / pow(2, depth + 2);
        pos.x += qWidth * ((pos.x >= 0) ? -1.0 : 1.0);
        pos.y += qWidth * ((pos.y >= 0) ? -1.0 : 1.0);
        pos.z += qWidth * ((pos.z >= 0) ? -1.0 : 1.0);

        depth++;
        currentOctreeNodeID = octreeNodes[currentOctreeNodeID].childrenIndices[childIndex];
    }
}

VoxelData getVoxelData(uint chunkDataIndex, dvec3 localPos, dvec3 rayDir, dvec3 invRayDir) {
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

        double deltaRay = getDeltaRay(localPos, iLocalPos, 1.0, rayDir, invRayDir) + deltaRayOffset;
        localPos += rayDir * deltaRay;
        voxelData.rayLength += deltaRay;
    }

    return voxelData;
}

vec3 getPixelColor(dvec3 pos, dvec3 rayDir, uint maxIterations) {
    dvec3 cameraPos = pos;
    double rayLength = 0.0;

    dvec3 invRayDir = 1.0 / rayDir;
    double hWorldWidth = u_worldWidth / 2.0;

    int iteration;
    for(iteration = 0; iteration < maxIterations; ++iteration) {
        if(pos.x <= -hWorldWidth || pos.x >= hWorldWidth || pos.y <= -hWorldWidth || pos.y >= hWorldWidth || pos.z <= -hWorldWidth || pos.z >= hWorldWidth) {
            break;
        }

        uint currentOctreeNodeID = 0;
        uint currentDepth = 0;
        dvec3 localPos = pos;
        getOctreeNode(currentOctreeNodeID, currentDepth, localPos);

        if(octreeNodes[currentOctreeNodeID].isSolidColor == 0) {
            if(currentDepth == u_maxOctreeDepth) {
                dvec3 localChunkPos = localPos + dvec3(u_chunkWidth, u_chunkWidth, u_chunkWidth) * 0.5;
                VoxelData voxelData = getVoxelData(octreeNodes[currentOctreeNodeID].dataIndex, localChunkPos, rayDir, invRayDir);
                if(voxelData.paletteIndex != 0) {
                    rayLength += voxelData.rayLength;
                    return abs(getNormal(voxelData));
                    // return u_palette[voxelData.paletteIndex];
                }
            }
        }
        else if(octreeNodes[currentOctreeNodeID].dataIndex != 0) {
            return u_palette[octreeNodes[currentOctreeNodeID].dataIndex];
        }

        double width = u_worldWidth / pow(2, currentDepth);
        ivec3 ipos = ivec3(floor(pos / width) * width);
        double deltaRay = getDeltaRay(pos, ipos, width, rayDir, invRayDir) + deltaRayOffset;
        pos += rayDir * deltaRay;

        rayLength += deltaRay;

    }

    double a = double(iteration) / 50.0;
    return vec3(a,a,a);
}

void main() {
    dvec3 pos = dvec3(u_cameraPos);

    float aspectRatio = u_windowSize.x / float(u_windowSize.y);
    vec2 screenSpaceCoordinates = fragPos - vec2(0.5, 0.5);

    dvec3 rayDirCamera;
    rayDirCamera.x = screenSpaceCoordinates.x * tan(u_fov) * aspectRatio;
    rayDirCamera.y = screenSpaceCoordinates.y * tan(u_fov);
    rayDirCamera.z = -1.0;
    rayDirCamera = normalize(rayDirCamera);

    dvec3 rayDir;
    rayDir.x = rayDirCamera.x * cos(u_cameraDir) - rayDirCamera.z * sin(u_cameraDir);
    rayDir.y = rayDirCamera.y;
    rayDir.z = rayDirCamera.x * sin(u_cameraDir) + rayDirCamera.z * cos(u_cameraDir);
    rayDir = normalize(rayDir);

    FragColor = vec4(getPixelColor(pos, rayDir, 100), 1.0);
}