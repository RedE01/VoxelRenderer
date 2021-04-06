#pragma once

enum class BufferDataUsage {
    STREAM_DRAW, STREAM_READ, STREAM_COPY, STATIC_DRAW, STATIC_READ, STATIC_COPY, DYNAMIC_DRAW, DYNAMIC_READ, DYNAMIC_COPY
};

class Buffer {
public:
    Buffer();
    ~Buffer();

    virtual void setData(void* data, unsigned int dataSize, BufferDataUsage usageType);

    virtual void bind();
    virtual void unbind();

    unsigned int getBufferID() const { return m_bufferID; }

private:
    virtual int getBufferType() = 0;

private:
    unsigned int m_bufferID;
};