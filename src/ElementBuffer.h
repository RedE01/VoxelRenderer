#pragma once
#include "Buffer.h"

class ElementBuffer : public Buffer {
private:
    virtual int getBufferType() override;
};