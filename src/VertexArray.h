#pragma once
#include <vector>

class VertexArray {
public:
    VertexArray();
    ~VertexArray();

    void bind();
    void unbind();

    unsigned int getBufferID() const { return m_vao; }

private:
    unsigned int m_vao;
};