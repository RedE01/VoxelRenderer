#include "Texture.h"
#include <GL/glew.h>

#include <iostream>

unsigned int getOpenGLTextureType(TextureType textureType);
unsigned int getOpenGLFilterMode(TextureFilterMode filterMode);
unsigned int getOpenGLWrapMode(TextureWrapMode wrapMode);
std::pair<int, int> getOpenGLTextureFormats(TextureFormat textureFormat);

Texture::Texture(TextureType textureType) {
    m_textureTypeID = getOpenGLTextureType(textureType);

    glGenTextures(1, &m_textureID);
}

Texture::~Texture() {
    glDeleteTextures(1, &m_textureID);    
}

void Texture::textureImage2D(TextureFormat textureFormat, unsigned int width, unsigned int height, char* data) {
    textureImage2dInternal(textureFormat, width, height, data, GL_BYTE);
}

void Texture::textureImage2D(TextureFormat textureFormat, unsigned int width, unsigned int height, unsigned char* data) {
    textureImage2dInternal(textureFormat, width, height, data, GL_UNSIGNED_BYTE);
}

void Texture::textureImage2D(TextureFormat textureFormat, unsigned int width, unsigned int height, short* data) {
    textureImage2dInternal(textureFormat, width, height, data, GL_SHORT);
}

void Texture::textureImage2D(TextureFormat textureFormat, unsigned int width, unsigned int height, unsigned short* data) {
    textureImage2dInternal(textureFormat, width, height, data, GL_UNSIGNED_SHORT);
}

void Texture::textureImage2D(TextureFormat textureFormat, unsigned int width, unsigned int height, int* data) {
    textureImage2dInternal(textureFormat, width, height, data, GL_INT);
}

void Texture::textureImage2D(TextureFormat textureFormat, unsigned int width, unsigned int height, unsigned int* data) {
    textureImage2dInternal(textureFormat, width, height, data, GL_UNSIGNED_INT);
}

void Texture::textureImage2D(TextureFormat textureFormat, unsigned int width, unsigned int height, float* data) {
    textureImage2dInternal(textureFormat, width, height, data, GL_FLOAT);
}

void Texture::bind() {
    glBindTexture(m_textureTypeID, m_textureID);
}

void Texture::unbind() {
    glBindTexture(m_textureTypeID, 0);
}

void Texture::setFilterMode(TextureFilterMode filterMode) {
    unsigned int filterModeID = getOpenGLFilterMode(filterMode);

    bind();
    glTexParameteri(m_textureTypeID, GL_TEXTURE_MIN_FILTER, filterModeID);
    glTexParameteri(m_textureTypeID, GL_TEXTURE_MAG_FILTER, filterModeID);
}

void Texture::setWrapModeS(TextureWrapMode wrapMode) {
    bind();
    glTexParameteri(m_textureTypeID, GL_TEXTURE_WRAP_S, getOpenGLWrapMode(wrapMode));
}

void Texture::setWrapModeT(TextureWrapMode wrapMode) {
    bind();
    glTexParameteri(m_textureTypeID, GL_TEXTURE_WRAP_T, getOpenGLWrapMode(wrapMode));
}

void Texture::setWrapModeR(TextureWrapMode wrapMode) {
    bind();
    glTexParameteri(m_textureTypeID, GL_TEXTURE_WRAP_R, getOpenGLWrapMode(wrapMode));
}

void Texture::textureImage2dInternal(TextureFormat textureFormat, unsigned int width, unsigned int height, const void* data, int type) {
    bind();
    std::pair<int, int> openGLformats = getOpenGLTextureFormats(textureFormat);
    glTexImage2D(m_textureTypeID, 0, openGLformats.first, width, height, 0, openGLformats.second, type, data);
    m_textureFormat = textureFormat;
}

unsigned int getOpenGLTextureType(TextureType textureType) {
    switch(textureType) {
        case TextureType::TEXTURE_2D: return GL_TEXTURE_2D;
        default: std::cout << "ERROR: Texture type not supported" << std::endl; break;
    }
    return 0;
}

unsigned int getOpenGLFilterMode(TextureFilterMode filterMode) {
    switch(filterMode) {
        case TextureFilterMode::NEAREST: return GL_NEAREST;
        case TextureFilterMode::LINEAR: return GL_LINEAR;
        default: std::cout << "Error: Texture filter mode not supported" << std::endl;
    }
    return 0;
}

unsigned int getOpenGLWrapMode(TextureWrapMode wrapMode) {
    switch(wrapMode) {
        case TextureWrapMode::CLAMP_TO_EDGE: return GL_CLAMP_TO_EDGE;
        case TextureWrapMode::CLAMP_TO_BORDER: return GL_CLAMP_TO_BORDER;
        case TextureWrapMode::MIRRORED_REPEAT: return GL_MIRRORED_REPEAT;
        case TextureWrapMode::REPEAT: return GL_REPEAT;
        case TextureWrapMode::MIRROR_CLAMP_TO_EDGE: return GL_MIRROR_CLAMP_TO_EDGE;
        default: std::cout << "Error: Texture wrap mode not supported" << std::endl;
    }
    return 0;
}

std::pair<int, int> getOpenGLTextureFormats(TextureFormat textureFormat) {
    int internalFormat, format;
    switch(textureFormat) {
        case TextureFormat::R8: internalFormat = GL_R8; format = GL_RED; break;
        case TextureFormat::R16: internalFormat = GL_R16; format = GL_RED; break;
        case TextureFormat::RG8: internalFormat = GL_RG8; format = GL_RG; break;
        case TextureFormat::RG16: internalFormat = GL_RG16; format = GL_RG; break;
        case TextureFormat::RGB8: internalFormat = GL_RGB8; format = GL_RGB; break;
        case TextureFormat::RGB12: internalFormat = GL_RGB12; format = GL_RGB; break;
        case TextureFormat::RGBA8: internalFormat = GL_RGBA8; format = GL_RGBA; break;
        case TextureFormat::RGBA16: internalFormat = GL_RGBA16; format = GL_RGBA; break;
        case TextureFormat::SRGB8: internalFormat = GL_SRGB8; format = GL_RGB; break;
        case TextureFormat::SRGB8_ALPHA8: internalFormat = GL_SRGB8_ALPHA8; format = GL_RGBA; break;
        case TextureFormat::R16F: internalFormat = GL_R16F; format = GL_RED; break;
        case TextureFormat::RG16F: internalFormat = GL_RG16F; format = GL_RG; break;
        case TextureFormat::RGB16F: internalFormat = GL_RGB16F; format = GL_RGB; break;
        case TextureFormat::RGBA16F: internalFormat = GL_RGBA16F; format = GL_RGBA; break;
        case TextureFormat::R32F: internalFormat = GL_R32F; format = GL_RED; break;
        case TextureFormat::RG32F: internalFormat = GL_RG32F; format = GL_RG; break;
        case TextureFormat::RGB32F: internalFormat = GL_RGB32F; format = GL_RGB; break;
        case TextureFormat::RGBA32F: internalFormat = GL_RGBA32F; format = GL_RGBA; break;
        case TextureFormat::R8I: internalFormat = GL_R8I; format = GL_RED_INTEGER; break;
        case TextureFormat::R8UI: internalFormat = GL_R8UI; format = GL_RED_INTEGER; break;
        case TextureFormat::R16I: internalFormat = GL_R16I; format = GL_RED_INTEGER; break;
        case TextureFormat::R16UI: internalFormat = GL_R16UI; format = GL_RED_INTEGER; break;
        case TextureFormat::R32I: internalFormat = GL_R32I; format = GL_RED_INTEGER; break;
        case TextureFormat::R32UI: internalFormat = GL_R32UI; format = GL_RED_INTEGER; break;
        case TextureFormat::RG8I: internalFormat = GL_RG8I; format = GL_RG_INTEGER; break;
        case TextureFormat::RG8UI: internalFormat = GL_RG8UI; format = GL_RG_INTEGER; break;
        case TextureFormat::RG16I: internalFormat = GL_RG16I; format = GL_RG_INTEGER; break;
        case TextureFormat::RG16UI: internalFormat = GL_RG16UI; format = GL_RG_INTEGER; break;
        case TextureFormat::RG32I: internalFormat = GL_RG32I; format = GL_RG_INTEGER; break;
        case TextureFormat::RG32UI: internalFormat = GL_RG32UI; format = GL_RG_INTEGER; break;
        case TextureFormat::RGB8I: internalFormat = GL_RGB8I; format = GL_RGB_INTEGER; break;
        case TextureFormat::RGB8UI: internalFormat = GL_RGB8UI; format = GL_RGB_INTEGER; break;
        case TextureFormat::RGB16I: internalFormat = GL_RGB16I; format = GL_RGB_INTEGER; break;
        case TextureFormat::RGB16UI: internalFormat = GL_RGB16UI; format = GL_RGB_INTEGER; break;
        case TextureFormat::RGB32I: internalFormat = GL_RGB32I; format = GL_RGB_INTEGER; break;
        case TextureFormat::RGB32UI: internalFormat = GL_RGB32UI; format = GL_RGB_INTEGER; break;
        case TextureFormat::RGBA8I: internalFormat = GL_RGBA8I; format = GL_RGBA_INTEGER; break;
        case TextureFormat::RGBA8UI: internalFormat = GL_RGBA8UI; format = GL_RGBA_INTEGER; break;
        case TextureFormat::RGBA16I: internalFormat = GL_RGBA16I; format = GL_RGBA_INTEGER; break;
        case TextureFormat::RGBA16UI: internalFormat = GL_RGBA16UI; format = GL_RGBA_INTEGER; break;
        case TextureFormat::RGBA32I: internalFormat = GL_RGBA32I; format = GL_RGBA_INTEGER; break;
        case TextureFormat::RGBA32UI: internalFormat = GL_RGBA32UI; format = GL_RGBA_INTEGER; break;
        case TextureFormat::STENCIL_INDEX: internalFormat = GL_STENCIL_INDEX; format = GL_STENCIL_INDEX; break; 
        case TextureFormat::DEPTH_COMPONENT: internalFormat = GL_DEPTH_COMPONENT; format = GL_DEPTH_COMPONENT; break; 
        case TextureFormat::DEPTH_STENCIL: internalFormat = GL_DEPTH_STENCIL; format = GL_DEPTH_STENCIL; break; 
        default: std::cout << "Texture format not supported" << std::endl;
    }

    return { internalFormat, format };
}