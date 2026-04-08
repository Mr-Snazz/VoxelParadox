// Arquivo: Engine/src/engine/shader.h
// Papel: declara o wrapper simples de shader usado pelo runtime.
// Fluxo: carrega fonte GLSL, compila programas e cacheia uniforms usados pelo renderer.
// DependÃªncias principais: OpenGL, GLM e `AppPaths`.
#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <unordered_map>

#include "path/app_paths.hpp"

class Shader {
public:
    GLuint program = 0;

    bool compile(const char* vertSrc, const char* fragSrc) {
        release();

        GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vertSrc, nullptr);
        glCompileShader(vs);
        if (!checkCompile(vs, "VERTEX")) return false;

        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fragSrc, nullptr);
        glCompileShader(fs);
        if (!checkCompile(fs, "FRAGMENT")) { glDeleteShader(vs); return false; }

        program = glCreateProgram();
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);
        GLint ok; glGetProgramiv(program, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[512]; glGetProgramInfoLog(program, 512, nullptr, log);
            printf("Shader link error: %s\n", log);
            glDeleteProgram(program); program = 0;
            uniformLocations.clear();
        }
        glDeleteShader(vs); glDeleteShader(fs);
        return program != 0;
    }

    bool compileFromFiles(const char* vertPath, const char* fragPath) {
        const std::string vertSource = readTextFile(vertPath);
        if (vertSource.empty()) {
            std::printf("Failed to read vertex shader file: %s\n", vertPath);
            return false;
        }

        const std::string fragSource = readTextFile(fragPath);
        if (fragSource.empty()) {
            std::printf("Failed to read fragment shader file: %s\n", fragPath);
            return false;
        }

        return compile(vertSource.c_str(), fragSource.c_str());
    }

    void use() const { glUseProgram(program); }

    void setMat4(const char* n, const glm::mat4& m) const {
        glUniformMatrix4fv(getUniformLocation(n), 1, GL_FALSE, glm::value_ptr(m));
    }
    void setVec3(const char* n, const glm::vec3& v) const {
        glUniform3fv(getUniformLocation(n), 1, glm::value_ptr(v));
    }
    void setVec4(const char* n, const glm::vec4& v) const {
        glUniform4fv(getUniformLocation(n), 1, glm::value_ptr(v));
    }
    void setFloat(const char* n, float f) const {
        glUniform1f(getUniformLocation(n), f);
    }
    void setInt(const char* n, int i) const {
        glUniform1i(getUniformLocation(n), i);
    }

    void release() {
        uniformLocations.clear();
        if (program) {
            glDeleteProgram(program);
            program = 0;
        }
    }

    ~Shader() { release(); }

private:
    mutable std::unordered_map<std::string, GLint> uniformLocations;

    static std::string readTextFile(const char* path) {
        if (!path || path[0] == '\0') {
            return {};
        }

        std::ifstream file(AppPaths::resolve(path), std::ios::in | std::ios::binary);
        if (!file.is_open()) {
            return {};
        }

        std::ostringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    bool checkCompile(GLuint s, const char* type) {
        GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[512]; glGetShaderInfoLog(s, 512, nullptr, log);
            printf("%s shader error: %s\n", type, log);
            glDeleteShader(s); return false;
        }
        return true;
    }

    GLint getUniformLocation(const char* n) const {
        if (!n || !*n || program == 0) {
            return -1;
        }

        const std::string name(n);
        auto found = uniformLocations.find(name);
        if (found != uniformLocations.end()) {
            return found->second;
        }

        const GLint location = glGetUniformLocation(program, n);
        uniformLocations.emplace(name, location);
        return location;
    }
};
