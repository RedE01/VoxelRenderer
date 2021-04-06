#pragma once

enum class VertexBufferDataUsage {
    STREAM_DRAW, STREAM_READ, STREAM_COPY, STATIC_DRAW, STATIC_READ, STATIC_COPY, DYNAMIC_DRAW, DYNAMIC_READ, DYNAMIC_COPY
};

class Buffer {
public:
    Buffer();
    ~Buffer();

    virtual void setData(void* data, unsigned int dataSize, VertexBufferDataUsage usageType);

    virtual void bind();
    virtual void unbind();

private:
    virtual int getBufferType() = 0;

private:
    unsigned int m_bufferID;
};