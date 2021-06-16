#include "Framebuffer.h"
#include "Texture.h"
#include <algorithm>
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

void Framebuffer::setDrawBuffers() {
    bind();
    glDrawBuffers(m_colorAttachments.size(), m_colorAttachments.data());
}

void Framebuffer::attachTexture(Texture* texture, unsigned int attachment) {
    bind();
    
    unsigned int attachmentPoint;
    if(texture->getTextureFormat() == TextureFormat::STENCIL_INDEX) attachmentPoint = GL_STENCIL_ATTACHMENT;
    else if(texture->getTextureFormat() == TextureFormat::DEPTH_COMPONENT) attachmentPoint = GL_DEPTH_ATTACHMENT;
    else if(texture->getTextureFormat() == TextureFormat::DEPTH_STENCIL) attachmentPoint = GL_DEPTH_STENCIL_ATTACHMENT;
    else {
        attachmentPoint = GL_COLOR_ATTACHMENT0 + attachment;
        if(std::find(m_colorAttachments.begin(), m_colorAttachments.end(), attachmentPoint) == m_colorAttachments.end()) {
            m_colorAttachments.insert(std::upper_bound(m_colorAttachments.begin(), m_colorAttachments.end(), attachmentPoint), attachmentPoint);
        }
    }

    glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentPoint, texture->getTextureTypeID(), texture->getTextureID(), 0);
}