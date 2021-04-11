#include "Framebuffer.h"
#include "Texture.h"
#include <GL/glew.h>

Framebuffer::Framebuffer() {
    glCreateFramebuffers(1, &m_framebufferID);
}

Framebuffer::~Framebuffer() {
    glDeleteFramebuffers(1, &m_framebufferID);
}

void Framebuffer::bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferID);
}

void Framebuffer::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::attachTexture(Texture* texture, unsigned int attachment) {
    bind();
    texture->bind();
    
    unsigned int attachmentPoint;
    if(texture->getTextureFormat() == TextureFormat::STENCIL_INDEX) attachmentPoint = GL_STENCIL_ATTACHMENT;
    else if(texture->getTextureFormat() == TextureFormat::DEPTH_COMPONENT) attachmentPoint = GL_DEPTH_ATTACHMENT;
    else if(texture->getTextureFormat() == TextureFormat::DEPTH_STENCIL) attachmentPoint = GL_DEPTH_STENCIL_ATTACHMENT;
    else attachmentPoint = GL_COLOR_ATTACHMENT0 + attachment;

    glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentPoint, texture->getTextureTypeID(), texture->getTextureID(), 0);
}