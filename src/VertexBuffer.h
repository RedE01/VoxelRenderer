#pragma once
#include "Buffer.h"
#include <vector>

enum class VertexAttributeType {
    BYTE, UNSIGNED_BYTE, SHORT, UNSIGNED_SHORT, INT, UNSIGNED_INT, FLOAT, DOUBLE
};

struct VertexAttribute {
    VertexAttribute(unsigned int size, VertexAttributeType type, bool normalized)
        : size(size), type(type), normalized(normalized) {}
    unsigned int size;
    VertexAttributeType type;
    bool normalized;
};

class VertexBuffer : public Buffer {
public:
    VertexBuffer(const std::vector<VertexAttribute>& attributes);

private:
    virtual int getBufferType() override;
};