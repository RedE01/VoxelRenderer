#include "Octree.h"
#include <cmath>
#include <cstdint>
#include <iostream>

Octree::Octree(uint8_t* world, unsigned int worldWidth, unsigned int maxDepth)
    : worldWidth(worldWidth), maxDepth(maxDepth) {

    nodes.push_back(OctreeNode(0));

    if(maxDepth > 0 && (worldWidth % (unsigned int)std::pow(2, maxDepth + 1) != 0)) {
        std::cout << "World width is not divisible by 2^(maxDepth + 1)" << std::endl;
        nodes[0].isSolidColor = 1;
    }
    else {
        initOctree(world, 0, 0, 0, 0, 0);
    }
}

void Octree::initOctree(uint8_t* world, unsigned int currentIndex, int depth, int startx, int starty, int startz) {
    int width = worldWidth / std::pow(2, depth);
    int hWidth = width / 2;
    
    if(!isSolidColor(world, width, startx, starty, startz)) {
        nodes[currentIndex].isSolidColor = 0;
        if(depth >= maxDepth) {
            initData(world, nodes[currentIndex], width, startx, starty, startz);
        }
        else {
            for(int i = 0; i < 8; ++i) {
                nodes[currentIndex].childrenIndices[i] = nodes.size();
                nodes.push_back(OctreeNode(currentIndex));

                int cStartx = startx + ((i % 2 == 0) ? 0 : hWidth);
                int cStarty = starty + ((i % 4 <  2) ? 0 : hWidth);
                int cStartz = startz + ((i % 8 <  4) ? 0 : hWidth);
                initOctree(world, nodes[currentIndex].childrenIndices[i], depth + 1, cStartx, cStarty, cStartz);
            }
        }
    }
    else {
        nodes[currentIndex].dataIndex = world[startx + starty * worldWidth + startz * worldWidth * worldWidth];
    }
}

bool Octree::isSolidColor(uint8_t* world, int width, int startx, int starty, int startz) {
        int endx = startx + width;
        int endy = starty + width;
        int endz = startz + width;
        uint8_t uniqueColor = world[startx + starty * worldWidth + startz * worldWidth * worldWidth];
        for(int z = startz; z < endz; z++) {
            for(int y = starty; y < endy; y++) {
                for(int x = startx; x < endx; x++) {
                    if(world[x + y * worldWidth + z * worldWidth * worldWidth] != uniqueColor) return false;
                }
            }
        }
        return true;
    }

void Octree::initData(uint8_t* world, OctreeNode& node, int width, int startx, int starty, int startz) {
    int endx = startx + width;
    int endy = starty + width;
    int endz = startz + width;
    node.dataIndex = chunkData.size();
    for(int z = startz; z < endz; z++) {
        for(int y = starty; y < endy; y++) {
            for(int x = startx; x < endx; x++) {
                chunkData.push_back(world[x + y * worldWidth + z * worldWidth * worldWidth]);
            }
        }
    }
}