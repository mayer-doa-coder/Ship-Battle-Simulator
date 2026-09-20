#pragma once

// Phase 2: a small shader-program loader.
// Phase 3: it can also send values from C++ into the running shader.
//
// Its job is to:
//   1. read a vertex-shader file and a fragment-shader file;
//   2. compile both files;
//   3. link them into one GPU program;
//   4. print useful errors instead of showing a silent black screen;
//   5. send uniform values to that program.

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

class ShaderProgram {
public:
    ShaderProgram() = default;
    ~ShaderProgram()
    {
        destroy();
    }

    // One object owns one OpenGL program. Copying it could delete the same
    // program twice, so copying is intentionally disabled.
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    bool loadFromFiles(const char* vertexPath, const char* fragmentPath)
    {
        destroy();

        std::string vertexSource;
        std::string fragmentSource;

        if (!readTextFile(vertexPath, vertexSource) ||
            !readTextFile(fragmentPath, fragmentSource)) {
            return false;
        }

        const GLuint vertexShader = compileShader(
            GL_VERTEX_SHADER,
            vertexSource,
            vertexPath);

        if (vertexShader == 0)
            return false;

        const GLuint fragmentShader = compileShader(
            GL_FRAGMENT_SHADER,
            fragmentSource,
            fragmentPath);

        if (fragmentShader == 0) {
            glDeleteShader(vertexShader);
            return false;
        }

        const GLuint program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        // After linking, the program contains the compiled result. The two
        // separate shader objects are no longer needed.
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        GLint linked = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        printProgramLog(program, vertexPath, fragmentPath, linked == GL_TRUE);

        if (linked != GL_TRUE) {
            glDeleteProgram(program);
            return false;
        }

        m_id = program;
        std::printf("[shader] linked program %u (%s + %s)\n",
                    m_id,
                    vertexPath,
                    fragmentPath);
        return true;
    }

    void use() const
    {
        glUseProgram(m_id);
    }

    // ---- Uniform setters -------------------------------------------------
    //
    // A uniform is one value that stays the same for every vertex and every
    // pixel of a draw call. It is how C++ talks to a shader while the program
    // is running, without editing or recompiling the .vert/.frag files.
    //
    // OpenGL writes uniforms into the program that is currently in use, so
    // call use() before calling any of these.

    void setInt(const char* name, int value)
    {
        glUniform1i(uniformLocation(name), value);
    }

    void setFloat(const char* name, float value)
    {
        glUniform1f(uniformLocation(name), value);
    }

    void setVec3(const char* name, const glm::vec3& value)
    {
        // Send 3 floats, read straight out of the glm vector.
        glUniform3fv(uniformLocation(name), 1, glm::value_ptr(value));
    }

    void setMat4(const char* name, const glm::mat4& value)
    {
        // Count 1 matrix, no transpose, then 16 floats.
        // glm already stores its matrices the way OpenGL expects, so the
        // transpose flag stays GL_FALSE. First used in Phase 4.
        glUniformMatrix4fv(uniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
    }

    bool valid() const
    {
        return m_id != 0;
    }

    void destroy()
    {
        if (m_id != 0) {
            glDeleteProgram(m_id);
            m_id = 0;
        }

        // A new program has new uniform locations, so old warnings no longer apply.
        m_missingUniforms.clear();
    }

private:
    GLuint m_id = 0;

    // Names we have already complained about. Without this the warning below
    // would print on every frame, roughly 120 times per second.
    std::vector<std::string> m_missingUniforms;

    // Ask the driver where a uniform lives inside the linked program.
    //
    // A result of -1 means the name was not found. That usually means one of:
    //   - the name is spelled differently in the shader;
    //   - the uniform is declared but never used, so the GLSL compiler
    //     removed it.
    // OpenGL ignores writes to location -1, so a wrong name fails silently.
    // That is why we print the warning once.
    GLint uniformLocation(const char* name)
    {
        const GLint location = glGetUniformLocation(m_id, name);

        if (location == -1) {
            const bool alreadyWarned =
                std::find(m_missingUniforms.begin(), m_missingUniforms.end(), name)
                != m_missingUniforms.end();

            if (!alreadyWarned) {
                m_missingUniforms.push_back(name);
                std::fprintf(
                    stderr,
                    "[shader] uniform '%s' not found or unused in program %u\n",
                    name,
                    m_id);
            }
        }

        return location;
    }

    static bool readTextFile(const char* path, std::string& text)
    {
        std::ifstream file(path, std::ios::in | std::ios::binary);
        if (!file) {
            std::fprintf(stderr, "[shader] could not open '%s'\n", path);
            return false;
        }

        std::ostringstream contents;
        contents << file.rdbuf();
        text = contents.str();

        if (text.empty()) {
            std::fprintf(stderr, "[shader] file is empty: '%s'\n", path);
            return false;
        }

        return true;
    }

    static GLuint compileShader(GLenum shaderType,
                                const std::string& source,
                                const char* path)
    {
        const GLuint shader = glCreateShader(shaderType);
        const char* sourcePointer = source.c_str();

        glShaderSource(shader, 1, &sourcePointer, nullptr);
        glCompileShader(shader);

        GLint compiled = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        printShaderLog(shader, path, compiled == GL_TRUE);

        if (compiled != GL_TRUE) {
            glDeleteShader(shader);
            return 0;
        }

        return shader;
    }

    static void printShaderLog(GLuint shader, const char* path, bool succeeded)
    {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

        if (logLength <= 1 && succeeded)
            return;

        std::vector<char> log(static_cast<std::size_t>(std::max(logLength, 1)));
        GLsizei written = 0;
        glGetShaderInfoLog(shader, logLength, &written, log.data());

        std::FILE* output = succeeded ? stdout : stderr;
        std::fprintf(output,
                     "[shader] compile %s: %s\n%s\n",
                     succeeded ? "warning" : "failed",
                     path,
                     written > 0 ? log.data() : "No driver log was provided.");
    }

    static void printProgramLog(GLuint program,
                                const char* vertexPath,
                                const char* fragmentPath,
                                bool succeeded)
    {
        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

        if (logLength <= 1 && succeeded)
            return;

        std::vector<char> log(static_cast<std::size_t>(std::max(logLength, 1)));
        GLsizei written = 0;
        glGetProgramInfoLog(program, logLength, &written, log.data());

        std::FILE* output = succeeded ? stdout : stderr;
        std::fprintf(output,
                     "[shader] link %s: %s + %s\n%s\n",
                     succeeded ? "warning" : "failed",
                     vertexPath,
                     fragmentPath,
                     written > 0 ? log.data() : "No driver log was provided.");
    }
};
