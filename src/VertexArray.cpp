#include "VertexArray.h"
#include <GL/glew.h>

VertexArray::VertexArray() {
    glGenVertexArrays(1, &m_vao);
    bind();
}
VertexArray::~VertexArray() {
    glDeleteVertexArrays(1, &m_vao);
}

void VertexArray::bind() {
    glBindVertexArray(m_vao);
}
void VertexArray::unbind() {
    glBindVertexArray(0);
}