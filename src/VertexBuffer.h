#pragma once
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

enum class VertexBufferDataUsage {
    STREAM_DRAW, STREAM_READ, STREAM_COPY, STATIC_DRAW, STATIC_READ, STATIC_COPY, DYNAMIC_DRAW, DYNAMIC_READ, DYNAMIC_COPY
};

class VertexBuffer {
public:
    VertexBuffer(const std::vector<VertexAttribute>& attributes);
    ~VertexBuffer();

    void setData(void* data, unsigned int dataSize, VertexBufferDataUsage usageType);

    void bind();
    void unbind();

private:
    unsigned int m_vbo;
};