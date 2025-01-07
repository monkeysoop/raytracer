#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <filesystem>
#include <vector>

class Mesh {
public:
    Mesh(const std::filesystem::path& filename, size_t material_id, glm::mat4 transform);
    ~Mesh();

    // using vec4 because glsl alligns vec3 to 16 bytes so for increased memmory usage it makes it faster and more convinient for me :)
    std::vector<glm::vec4> m_vertecies; 
    std::vector<glm::vec4> m_normals;
    std::vector<glm::uvec4> m_triangles;
private:
};