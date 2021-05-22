#pragma once

enum class TextureType {
    TEXTURE_2D
};

enum class TextureFilterMode {
    NEAREST, LINEAR
};

enum class TextureWrapMode {
    CLAMP_TO_EDGE, CLAMP_TO_BORDER, MIRRORED_REPEAT, REPEAT, MIRROR_CLAMP_TO_EDGE
};

enum class TextureFormat {
    R8, R16, RG8, RG16, RGB8, RGB12, RGBA8, RGBA16, SRGB8, SRGB8_ALPHA8, R16F, RG16F, RGB16F, RGBA16F, R32F, RG32F, RGB32F,
    RGBA32F, R8I, R8UI, R16I, R16UI, R32I, R32UI, RG8I, RG8UI, RG16I, RG16UI, RG32I, RG32UI, RGB8I, RGB8UI, RGB16I,
    RGB16UI, RGB32I, RGB32UI, RGBA8I, RGBA8UI, RGBA16I, RGBA16UI, RGBA32I, RGBA32UI, STENCIL_INDEX, DEPTH_COMPONENT, DEPTH_STENCIL
};

class Texture {
public:
    Texture(TextureType textureType);
    ~Texture();

    void textureImage2D(const char* filename, unsigned int channels = 0);
    void textureImage2D(TextureFormat textureFormat, unsigned int width, unsigned int height, char* data);
    void textureImage2D(TextureFormat textureFormat, unsigned int width, unsigned int height, unsigned char* data);
    void textureImage2D(TextureFormat textureFormat, unsigned int width, unsigned int height, short* data);
    void textureImage2D(TextureFormat textureFormat, unsigned int width, unsigned int height, unsigned short* data);
    void textureImage2D(TextureFormat textureFormat, unsigned int width, unsigned int height, int* data);
    void textureImage2D(TextureFormat textureFormat, unsigned int width, unsigned int height, unsigned int* data);
    void textureImage2D(TextureFormat textureFormat, unsigned int width, unsigned int height, float* data);

    void bind();
    void unbind();

    void setFilterMode(TextureFilterMode filterMode);
    void setWrapModeS(TextureWrapMode wrapMode);
    void setWrapModeT(TextureWrapMode wrapMode);
    void setWrapModeR(TextureWrapMode wrapMode);

    unsigned int getTextureID() const { return m_textureID; }
    unsigned int getTextureTypeID() const { return m_textureTypeID; }
    TextureFormat getTextureFormat() const { return m_textureFormat; }

private:
    void textureImage2dInternal(TextureFormat textureFormat, unsigned int width, unsigned int height, const void* data, int type);

private:
    unsigned int m_textureID;
    unsigned int m_textureTypeID;
    TextureFormat m_textureFormat;
};