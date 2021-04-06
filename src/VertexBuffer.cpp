#include "VertexBuffer.h"
#include <GL/glew.h>

unsigned int getDataTypeSize(VertexAttributeType attributeType);
GLenum getOpenGLDataType(VertexAttributeType attributeType);

VertexBuffer::VertexBuffer(const std::vector<VertexAttribute>& attributes) : Buffer() {
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

int VertexBuffer::getBufferType() {
    return GL_ARRAY_BUFFER;
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