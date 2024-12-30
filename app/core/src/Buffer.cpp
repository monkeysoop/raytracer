#include "Buffer.hpp"

Buffer::Buffer(GLsizeiptr size, const void* data, GLbitfield flags) {
    glCreateBuffers(1, &m_buffer_id);
    glNamedBufferStorage(m_buffer_id, size, data, flags);
}

Buffer::~Buffer() {
    glDeleteBuffers(1, &m_buffer_id);
}

void Buffer::Bind(GLuint index) {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, m_buffer_id);
}