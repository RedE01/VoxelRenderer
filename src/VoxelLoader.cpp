#include "VoxelLoader.h"
#include <fstream>
#include <sstream>
#include <cstring>
#include <iostream>

namespace VoxelLoader {

    VoxelData parseVoxelFile(uint8_t* fileBuffer, unsigned int fileSize, VoxelDataAxis axis);
    VoxelData parseXRAWFile(uint8_t* fileBuffer, unsigned int fileSize, VoxelDataAxis axis);

    VoxelData loadVoxelData(const char* filename, VoxelDataAxis axis) {
        std::ifstream file(filename, std::ios::binary | std::ios::in | std::ios::ate);
        if(!file.is_open()) {
            std::cout << "Could not open file " << filename << std::endl;
            return {nullptr, 0, 0, 0, nullptr};
        }

        std::streampos fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        uint8_t* fileBuffer = new uint8_t[fileSize];
        file.read((char*)fileBuffer, fileSize);
        
        file.close();

        VoxelData res = parseVoxelFile(fileBuffer, fileSize, axis);

        delete[] fileBuffer;

        return res;
    }

    VoxelData parseVoxelFile(uint8_t* fileBuffer, unsigned int fileSize, VoxelDataAxis axis) {
        if(fileSize < 4) return {nullptr, 0, 0, 0, nullptr};

        if(fileBuffer[0] == 'X' && fileBuffer[1] == 'R' && fileBuffer[2] == 'A' && fileBuffer[3] == 'W'){
            return parseXRAWFile(fileBuffer, fileSize, axis);
        }
        else {
            std::cout << "unsuported file type" << std::endl;
            return {nullptr, 0, 0, 0, nullptr};
        }
    }

    VoxelData parseXRAWFile(uint8_t* fileBuffer, unsigned int fileSize, VoxelDataAxis axis) {
        struct Header {
            uint32_t XRAW;
            uint8_t colorChannelDataType;
            uint8_t numOfColorChannels;
            uint8_t bitsPerChannel;
            uint8_t bitsPerIndex;
            uint32_t x;
            uint32_t y;
            uint32_t z;
            uint32_t numOfPaletteColors;
        };

        if(fileSize < sizeof(Header)) return {nullptr, 0, 0, 0, nullptr};

        Header header;
        header = *((Header*)fileBuffer);
        
        std::cout << (int)header.colorChannelDataType << ", " << (int)header.numOfColorChannels << ", " << (int)header.bitsPerChannel << ", " << (int)header.bitsPerIndex << ", " << (int)header.x << ", " << (int)header.y << ", " << (int)header.z << ", " << (int)header.numOfPaletteColors << std::endl; 
        if(header.numOfPaletteColors != 256) {
            std::cout << "ERROR: Number of palette colors must be 256" << std::endl;
            return {nullptr, 0, 0, 0, nullptr};
        }

        unsigned int voxelDataSize = header.x * header.y * header.z * (header.bitsPerIndex / 8);
        unsigned int paletteDataSize = header.numOfColorChannels * (header.bitsPerChannel / 8) * header.numOfPaletteColors;
        if(fileSize < sizeof(Header) + voxelDataSize + paletteDataSize) {
            std::cout << "ERROR: File too small" << std::endl;
            return {nullptr, 0, 0, 0, nullptr};
        }

        uint8_t* voxelDataBuffer = new uint8_t[voxelDataSize];

        if(axis == VoxelDataAxis::Z_Up) {
            for(int z = 0; z < header.z; ++z) {
                for(int y = 0; y < header.y; ++y) {
                    for(int x = 0; x < header.x; ++x) {
                        int srcIndex = sizeof(Header) + x + z * header.x + y * header.x * header.z;
                        int destIndex = x + y * header.x + z * header.x * header.y;
                        voxelDataBuffer[destIndex] = fileBuffer[srcIndex];
                    }
                }
            }
        }
        else {
            std::memcpy(voxelDataBuffer, fileBuffer + sizeof(Header), voxelDataSize);
        }

        float* paletteData = new float[256 * 3];
        unsigned int paletteStartIndex = sizeof(Header) + voxelDataSize;
        unsigned int usedColorChannels = std::min((int)header.numOfColorChannels, 3);
        unsigned int bytesPerChannels = header.bitsPerChannel / 8;
        for(unsigned int colorIndex = 0; colorIndex < 256; ++colorIndex) {
            for(unsigned int channelIndex = 0; channelIndex < usedColorChannels; ++channelIndex) {
                uint8_t* sourcePtr = fileBuffer + paletteStartIndex + (colorIndex * header.numOfColorChannels + channelIndex) * bytesPerChannels;
                unsigned int destIndex = colorIndex * 3 + channelIndex;

                float res = 0;
                switch(header.colorChannelDataType) {
                case 0: // Unsigned integer
                    switch(bytesPerChannels) {
                    case 1: res = *((uint8_t*)sourcePtr) / 255.0; break;
                    case 2: res = *((uint16_t*)sourcePtr) / 255.0; break;
                    case 4: res = *((uint32_t*)sourcePtr) / 255.0; break;
                    }
                    break;
                case 1: // Signed integer
                    switch(bytesPerChannels) {
                    case 1: res = *((int8_t*)sourcePtr) / 255.0; break;
                    case 2: res = *((int16_t*)sourcePtr) / 255.0; break;
                    case 4: res = *((int32_t*)sourcePtr) / 255.0; break;
                    }
                    break;
                case 3: // Float
                    if(bytesPerChannels == 4) res = *((float*)sourcePtr);
                    break;
                }

                paletteData[destIndex] = res;
            }
        }
        

        return {voxelDataBuffer, header.x, header.y, header.z, paletteData};
    }

}