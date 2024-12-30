#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <filesystem>
#include <vector>

class Mesh {
public:
    Mesh(const std::filesystem::path& filename);
    ~Mesh();

    std::vector<glm::vec3> GetVertecies();
    std::vector<glm::uvec3> GetIndecies();

private:
    std::vector<glm::vec3> m_vertecies;
    std::vector<glm::uvec3> m_triangle_indecies;
};