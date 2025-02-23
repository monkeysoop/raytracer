#pragma once

#include <GL/glew.h>


class Buffer {
public:
    Buffer(GLsizeiptr size, const void* data);
    ~Buffer();

    void Bind(GLuint index);

private:
    GLuint m_buffer_id;
};