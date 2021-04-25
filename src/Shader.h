#pragma once
#include <unordered_map>
#include <string>
#include <memory>
#include <glm/glm.hpp>

class Texture;

class Shader {
public:
    Shader(const char* filepath);
    ~Shader();

    void useShader();
    void bindTextures();

    void setUniform1f(const char* name, const float& v);
    void setUniform2f(const char* name, const float& v1, const float& v2);
    void setUniform3f(const char* name, const float& v1, const float& v2, const float& v3);
    void setUniform4f(const char* name, const float& v1, const float& v2, const float& v3, const float& v4);
    void setUniform1i(const char* name, const int& v);
    void setUniform2i(const char* name, const int& v1, const int& v2);
    void setUniform3i(const char* name, const int& v1, const int& v2, const int& v3);
    void setUniform4i(const char* name, const int& v1, const int& v2, const int& v3, const int& v4);
    void setUniform1ui(const char* name, const unsigned int& v);
    void setUniform2ui(const char* name, const unsigned int& v1, const unsigned int& v2);
    void setUniform3ui(const char* name, const unsigned int& v1, const unsigned int& v2, const unsigned int& v3);
    void setUniform4ui(const char* name, const unsigned int& v1, const unsigned int& v2, const unsigned int& v3, const unsigned int& v4);
    void setUniform1fv(const char* name, const unsigned int count, const float* v);
    void setUniform2fv(const char* name, const unsigned int count, const float* v);
    void setUniform3fv(const char* name, const unsigned int count, const float* v);
    void setUniform4fv(const char* name, const unsigned int count, const float* v);
    void setUniform1iv(const char* name, const unsigned int count, const int* v);
    void setUniform2iv(const char* name, const unsigned int count, const int* v);
    void setUniform3iv(const char* name, const unsigned int count, const int* v);
    void setUniform4iv(const char* name, const unsigned int count, const int* v);
    void setUniform1uiv(const char* name, const unsigned int count, const unsigned int* v);
    void setUniform2uiv(const char* name, const unsigned int count, const unsigned int* v);
    void setUniform3uiv(const char* name, const unsigned int count, const unsigned int* v);
    void setUniform4uiv(const char* name, const unsigned int count, const unsigned int* v);
    void setUniformMat2(const char* name, const glm::mat2& matrix);
    void setUniformMat3(const char* name, const glm::mat3& matrix);
    void setUniformMat4(const char* name, const glm::mat4& matrix);

    void setTexture(std::weak_ptr<Texture> texture, unsigned int target, const char* samplerUniformName);

    unsigned int getShaderProgramID() const { return m_shaderProgramID; }

private:
    int getUniformLocation(const char* name);

private:
    unsigned int m_shaderProgramID;

    std::unordered_map<const char*, int> m_uniformLocations;
    std::unordered_map<unsigned int, std::weak_ptr<Texture>> m_textures;
};