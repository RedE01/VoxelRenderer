#pragma once
#include <vector>
#include <cstdint>

struct OctreeNode {
    OctreeNode(unsigned int parentIndex) : parentIndex(parentIndex), childrenIndices{0, 0, 0, 0, 0, 0, 0, 0}, dataIndex(0), isSolidColor(1) {}
    const unsigned int parentIndex;
    unsigned int childrenIndices[8];
    int isSolidColor;
    unsigned int dataIndex;
};

class Octree {
public:
    Octree(uint8_t* world, unsigned int worldWidth, unsigned int maxDepth);

private:
    void initOctree(uint8_t* world, unsigned int currentIndex, int depth, int startx, int starty, int startz);
    bool isSolidColor(uint8_t* world, int width, int startx, int starty, int startz);
    void initData(uint8_t* world, OctreeNode& node, int width, int startx, int starty, int startz);

public:
    const unsigned int worldWidth;
    const unsigned int maxDepth;
    
    std::vector<uint8_t> chunkData;
    std::vector<OctreeNode> nodes;
};