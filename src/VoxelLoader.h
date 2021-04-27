#pragma once
#include <cstdint>

enum class VoxelDataAxis {
    Y_Up, Z_Up
};

struct VoxelData {
    uint8_t* voxelData;
    unsigned int voxelDataSize;
    float* paletteData;
};

namespace VoxelLoader {

    VoxelData loadVoxelData(const char* filename, VoxelDataAxis axis = VoxelDataAxis::Y_Up);

}