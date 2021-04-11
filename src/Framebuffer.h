#pragma once

class Texture;

class Framebuffer {
public:
    Framebuffer();
    ~Framebuffer();

    void bind();
    void unbind();

    void attachTexture(Texture* texture, unsigned int attachment);

    unsigned int getFramebufferID() const { return m_framebufferID; }

private:
    unsigned int m_framebufferID;
};