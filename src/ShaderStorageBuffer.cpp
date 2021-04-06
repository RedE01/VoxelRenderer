#include "ShaderStorageBuffer.h"
#include <GL/glew.h>

ShaderStorageBuffer::ShaderStorageBuffer(unsigned int index) : Buffer() {
    bind();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, getBufferID());
}
    
int ShaderStorageBuffer::getBufferType() {
    return GL_SHADER_STORAGE_BUFFER;
}