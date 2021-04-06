#pragma once
#include "Buffer.h"

class ShaderStorageBuffer : public Buffer {
public:
    ShaderStorageBuffer(unsigned int index);
    
private:
    virtual int getBufferType() override;
};
