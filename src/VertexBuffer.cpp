#include "VertexBuffer.h"
#include <GL/glew.h>

unsigned int getDataTypeSize(VertexAttributeType attributeType);
GLenum getOpenGLDataType(VertexAttributeType attributeType);

VertexBuffer::VertexBuffer(const std::vector<VertexAttribute>& attributes) {
    glGenBuffers(1, &m_vbo);
    bind();

    unsigned int attributesStride = 0;
    for(int i = 0; i < attributes.size(); ++i) {
        unsigned int dataTypeSize = getDataTypeSize(attributes[i].type);
        attributesStride += attributes[i].size * dataTypeSize;
    }

    unsigned int currentOffset = 0;
    for(int i = 0; i < attributes.size(); ++i) {
        GLenum type = getOpenGLDataType(attributes[i].type);
        unsigned int dataTypeSize = getDataTypeSize(attributes[i].type);
        GLboolean normalized = attributes[i].normalized ? GL_TRUE : GL_FALSE;

        glVertexAttribPointer(i, attributes[i].size, type, normalized, attributesStride, (void*)currentOffset);
        glEnableVertexAttribArray(i);
    }
}

VertexBuffer::~VertexBuffer() {
    glDeleteBuffers(1, &m_vbo);
}

void VertexBuffer::setData(void* data, unsigned int dataSize, VertexBufferDataUsage usageType) {
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
    
    glBufferData(GL_ARRAY_BUFFER, dataSize, data, glUsage);
}

void VertexBuffer::bind() {
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
}

void VertexBuffer::unbind() {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

unsigned int getDataTypeSize(VertexAttributeType attributeType) {
    switch(attributeType) {
        case VertexAttributeType::BYTE: return sizeof(char);
        case VertexAttributeType::UNSIGNED_BYTE: return sizeof(unsigned char);
        case VertexAttributeType::SHORT: return sizeof(short);
        case VertexAttributeType::UNSIGNED_SHORT: return sizeof(short);
        case VertexAttributeType::INT: return sizeof(int);
        case VertexAttributeType::UNSIGNED_INT: return sizeof(unsigned int);
        case VertexAttributeType::FLOAT: return sizeof(float);
        case VertexAttributeType::DOUBLE: return sizeof(double);
    }
    return 0;
}

GLenum getOpenGLDataType(VertexAttributeType attributeType) {
    switch(attributeType) {
        case VertexAttributeType::BYTE: return GL_BYTE;
        case VertexAttributeType::UNSIGNED_BYTE: return GL_UNSIGNED_BYTE;
        case VertexAttributeType::SHORT: return GL_SHORT;
        case VertexAttributeType::UNSIGNED_SHORT: return GL_UNSIGNED_SHORT;
        case VertexAttributeType::INT: return GL_INT;
        case VertexAttributeType::UNSIGNED_INT: return GL_UNSIGNED_INT;
        case VertexAttributeType::FLOAT: return GL_FLOAT;
        case VertexAttributeType::DOUBLE: return GL_DOUBLE;
    }
    return 0;
}