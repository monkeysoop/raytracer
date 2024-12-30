#pragma once

#include <GL/glew.h>

#include <filesystem>

class Shader {
public:
    Shader(const std::filesystem::path& vs_filename, const std::filesystem::path& fs_filename);
    ~Shader();

    void Use();
    GLint ul(const GLchar* name);
    GLuint GetProgramID();

private:
    GLuint m_program_id;
};