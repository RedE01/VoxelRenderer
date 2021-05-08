#include "Shader.h"
#include "Texture.h"
#include <fstream>
#include <sstream>
#include <cstring>
#include <iostream>
#include <algorithm>

#include <GL/glew.h>

std::unordered_map<std::string, std::string> getSourcesFromFile(const char* filepath);
int createShader(const std::string& source, GLenum shaderType);
void deleteShader(unsigned int shaderID);
int createShaderProgram(unsigned int vertexShaderID, unsigned int fragmentShaderID);

Shader::Shader(const char* filepath) {
    std::unordered_map<std::string, std::string> sources = getSourcesFromFile(filepath);

    auto vertexShaderSource = sources.find("vertex");
    auto fragmentShaderSource = sources.find("fragment");

    if(vertexShaderSource != sources.end() && fragmentShaderSource != sources.end()) {
        int vertexShaderID = createShader(vertexShaderSource->second, GL_VERTEX_SHADER);
        int fragmentShaderID = createShader(fragmentShaderSource->second, GL_FRAGMENT_SHADER);
        if(vertexShaderID < 0 || fragmentShaderID < 0) {
            m_compiled = false;
            return;
        }

        m_shaderProgramID = createShaderProgram(vertexShaderID, fragmentShaderID);
        useShader();

        deleteShader(vertexShaderID);
        deleteShader(fragmentShaderID);
    }
    else {
        std::cout << "ERROR: No vertex and/or fragment shader" << std::endl;
        m_compiled = false;
        return;
    }
    m_compiled = true;
}

Shader::~Shader() {
    glDeleteProgram(m_shaderProgramID);
}

void Shader::useShader() {
    glUseProgram(m_shaderProgramID);
}

void Shader::bindTextures() {
    for(auto it = m_textures.begin(); it != m_textures.end();) {
        Texture* texture = it->second.lock().get();
        if(texture) {
            glActiveTexture(GL_TEXTURE0 + it->first);
            texture->bind();
            ++it;
        }
        else {
            it = m_textures.erase(it);
        }
    }
}

void Shader::setUniform1f(const char* name, const float& v) {
    glUniform1f(getUniformLocation(name), v);
}

void Shader::setUniform2f(const char* name, const float& v1, const float& v2) {
    glUniform2f(getUniformLocation(name), v1, v2);
}

void Shader::setUniform3f(const char* name, const float& v1, const float& v2, const float& v3) {
    glUniform3f(getUniformLocation(name), v1, v2, v3);
}

void Shader::setUniform4f(const char* name, const float& v1, const float& v2, const float& v3, const float& v4) {
    glUniform4f(getUniformLocation(name), v1, v2, v3, v4);
}

void Shader::setUniform1i(const char* name, const int& v) {
    glUniform1i(getUniformLocation(name), v);
}

void Shader::setUniform2i(const char* name, const int& v1, const int& v2) {
    glUniform2i(getUniformLocation(name), v1, v2);
}

void Shader::setUniform3i(const char* name, const int& v1, const int& v2, const int& v3) {
    glUniform3i(getUniformLocation(name), v1, v2, v3);
}

void Shader::setUniform4i(const char* name, const int& v1, const int& v2, const int& v3, const int& v4) {
    glUniform4i(getUniformLocation(name), v1, v2, v3, v4);
}

void Shader::setUniform1ui(const char* name, const unsigned int& v) {
    glUniform1ui(getUniformLocation(name), v);
}

void Shader::setUniform2ui(const char* name, const unsigned int& v1, const unsigned int& v2) {
    glUniform2ui(getUniformLocation(name), v1, v2);
}

void Shader::setUniform3ui(const char* name, const unsigned int& v1, const unsigned int& v2, const unsigned int& v3) {
    glUniform3ui(getUniformLocation(name), v1, v2, v3);
}

void Shader::setUniform4ui(const char* name, const unsigned int& v1, const unsigned int& v2, const unsigned int& v3, const unsigned int& v4) {
    glUniform4ui(getUniformLocation(name), v1, v2, v3, v4);
}

void Shader::setUniform1fv(const char* name, const unsigned int count, const float* v) {
    glUniform1fv(getUniformLocation(name), count, v);
}

void Shader::setUniform2fv(const char* name, const unsigned int count, const float* v) {
    glUniform2fv(getUniformLocation(name), count, v);
}

void Shader::setUniform3fv(const char* name, const unsigned int count, const float* v) {
    glUniform3fv(getUniformLocation(name), count, v);
}

void Shader::setUniform4fv(const char* name, const unsigned int count, const float* v) {
    glUniform4fv(getUniformLocation(name), count, v);
}

void Shader::setUniform1iv(const char* name, const unsigned int count, const int* v) {
    glUniform1iv(getUniformLocation(name), count, v);
}

void Shader::setUniform2iv(const char* name, const unsigned int count, const int* v) {
    glUniform2iv(getUniformLocation(name), count, v);
}

void Shader::setUniform3iv(const char* name, const unsigned int count, const int* v) {
    glUniform3iv(getUniformLocation(name), count, v);
}

void Shader::setUniform4iv(const char* name, const unsigned int count, const int* v) {
    glUniform4iv(getUniformLocation(name), count, v);
}

void Shader::setUniform1uiv(const char* name, const unsigned int count, const unsigned int* v) {
    glUniform1uiv(getUniformLocation(name), count, v);
}

void Shader::setUniform2uiv(const char* name, const unsigned int count, const unsigned int* v) {
    glUniform2uiv(getUniformLocation(name), count, v);
}

void Shader::setUniform3uiv(const char* name, const unsigned int count, const unsigned int* v) {
    glUniform3uiv(getUniformLocation(name), count, v);
}

void Shader::setUniform4uiv(const char* name, const unsigned int count, const unsigned int* v) {
    glUniform4uiv(getUniformLocation(name), count, v);
}

void Shader::setUniformMat2(const char* name, const glm::mat2& matrix) {
    glUniformMatrix2fv(getUniformLocation(name), 1, GL_FALSE, &(matrix[0][0]));
}

void Shader::setUniformMat3(const char* name, const glm::mat3& matrix) {
    glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, &(matrix[0][0]));
}

void Shader::setUniformMat4(const char* name, const glm::mat4& matrix) {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, &(matrix[0][0]));
}

void Shader::setTexture(std::weak_ptr<Texture> texture, unsigned int target, const char* samplerUniformName) {
    m_textures[target] = texture;
    setUniform1i(samplerUniformName, target);
}

int Shader::getUniformLocation(const char* name) {
    auto search = m_uniformLocations.find(name);
    if(search != m_uniformLocations.end()) {
        return search->second;
    }

    int uniformLocation = glGetUniformLocation(m_shaderProgramID, name);
    m_uniformLocations.insert(std::pair<const char*, int>(name, uniformLocation));

    return uniformLocation;
}

std::unordered_map<std::string, std::string> getSourcesFromFile(const char* filepath) {
    std::unordered_map<std::string, std::string> shaderSections;
    
    std::ifstream filestream(filepath, std::ios::in);
    if(!filestream.is_open()) {
        std::cout << "Could not open file " << filepath << std::endl;
        return shaderSections;
    }
    std::stringstream buffer;
    buffer << filestream.rdbuf();
    filestream.close();
    
    std::string shaderSource = buffer.str();

    const char* sectionToken = "#section";
    size_t sectionTokenLength = std::strlen(sectionToken);
    size_t pos = shaderSource.find(sectionToken, 0);
    while(pos != std::string::npos) {
        size_t eol = shaderSource.find_first_of("\r\n", pos);
        size_t sectionTypeBegin = pos + sectionTokenLength + 1;
        std::string sectionStr = shaderSource.substr(sectionTypeBegin, eol - sectionTypeBegin);
        
        size_t nextLinePos = shaderSource.find_first_of("\r\n", eol);
        pos = shaderSource.find(sectionToken, nextLinePos);

        shaderSections[sectionStr] = (pos == std::string::npos) ? shaderSource.substr(nextLinePos) : shaderSource.substr(nextLinePos, pos - nextLinePos);
    }

    return shaderSections;
}

int createShader(const std::string& source, GLenum shaderType) {
    unsigned int shaderID = glCreateShader(shaderType);

    const char* shaderSourceCstr = source.c_str();
    glShaderSource(shaderID, 1, &shaderSourceCstr, NULL);
    glCompileShader(shaderID);

    int  success;
    char infoLog[512];
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);

    if(!success) {
        glGetShaderInfoLog(shaderID, 512, NULL, infoLog);
        std::cout << "ERROR: SHADER COMPILATION FAILED\n" << infoLog << std::endl;
        return -1;
    }

    return shaderID;
}

void deleteShader(unsigned int shaderID) {
    glDeleteShader(shaderID);
}

int createShaderProgram(unsigned int vertexShaderID, unsigned int fragmentShaderID) {
    unsigned int shaderProgramID = glCreateProgram();

    glAttachShader(shaderProgramID, vertexShaderID);
    glAttachShader(shaderProgramID, fragmentShaderID);
    glLinkProgram(shaderProgramID);

    int  success;
    char infoLog[512];
    glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &success);

    if(!success) {
        glGetProgramInfoLog(shaderProgramID, 512, NULL, infoLog);
        std::cout << "ERROR: SHADER PROGRAM LINKING FAILED\n" << infoLog << std::endl;
    }

    return shaderProgramID;
}