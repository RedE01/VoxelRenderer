#include "Buffer.h"
#include <GL/glew.h>

Buffer::Buffer() {
    glGenBuffers(1, &m_bufferID);
}

Buffer::~Buffer() {
    glDeleteBuffers(1, &m_bufferID);
}

void Buffer::setData(void* data, unsigned int dataSize, VertexBufferDataUsage usageType) {
    bind();

    GLenum glUsage = GL_STATIC_DRAW;
    switch(usageType) {
        case VertexBufferDataUsage::STREAM_DRAW: glUsage = GL_STREAM_DRAW; break;
        case VertexBufferDataUsage::STREAM_READ: glUsage = GL_STREAM_READ; break;
        case VertexBufferDataUsage::STREAM_COPY: glUsage = GL_STREAM_COPY; break;
        case VertexBufferDataUsage::STATIC_DRAW: glUsage = GL_STATIC_DRAW; break;
        case VertexBufferDataUsage::STATIC_READ: glUsage = GL_STATIC_READ; break;
        case VertexBufferDataUsage::STATIC_COPY: glUsage = GL_STATIC_COPY; break;
        case VertexBufferDataUsage::DYNAMIC_DRAW: glUsage = GL_DYNAMIC_DRAW; break;
        case VertexBufferDataUsage::DYNAMIC_READ: glUsage = GL_DYNAMIC_READ; break;
        case VertexBufferDataUsage::DYNAMIC_COPY: glUsage = GL_DYNAMIC_COPY; break;
    }

    glBufferData(getBufferType(), dataSize, data, glUsage);
}

void Buffer::bind() {
    glBindBuffer(getBufferType(), m_bufferID);
}

void Buffer::unbind() {
    glBindBuffer(getBufferType(), 0);
}