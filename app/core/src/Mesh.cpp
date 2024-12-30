#include "Mesh.hpp"

#include <SDL2/SDL.h>
#include <string>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>


Mesh::Mesh(const std::filesystem::path& filename) {
    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(filename)) {
        if (!reader.Error().empty()) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Mesh] TinyObjReader loading error: %s", reader.Error().c_str());
        }
        return;
    }

    if (!reader.Warning().empty()) {
        SDL_LogMessage(SDL_LOG_CATEGORY_ERROR, SDL_LOG_PRIORITY_WARN, "[Mesh] TinyObjReader loading warning: %s", reader.Warning().c_str());
    }

    const tinyobj::attrib_t& attrib = reader.GetAttrib();
    const std::vector<tinyobj::shape_t>& shapes = reader.GetShapes();

    for (size_t i = 0; i < attrib.vertices.size(); i += 3) {
        GLfloat x = static_cast<GLfloat>(attrib.vertices[i + 0]);
        GLfloat y = static_cast<GLfloat>(attrib.vertices[i + 1]);
        GLfloat z = static_cast<GLfloat>(attrib.vertices[i + 2]);

        m_vertecies.push_back(glm::vec3{x, y, z});
    }

    for (const tinyobj::shape_t& shape : shapes) {
        size_t i = 0;

        for (const size_t fv : shape.mesh.num_face_vertices) {
            if (fv != 3) {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Mesh] error, only triangle faces are supported");    // tinyobjloader supposedly automatically converts all polygons to triangles if not specified otherwise
                return;
            }

            GLuint index_1 = static_cast<GLuint>(shape.mesh.indices[i + 0].vertex_index);
            GLuint index_2 = static_cast<GLuint>(shape.mesh.indices[i + 1].vertex_index);
            GLuint index_3 = static_cast<GLuint>(shape.mesh.indices[i + 2].vertex_index);

            m_triangle_indecies.push_back(glm::uvec3(index_1, index_2, index_3));

            i += 3;
        }
    }
}

Mesh::~Mesh() {}

std::vector<glm::vec3> Mesh::GetVertecies() {
    return m_vertecies;
}

std::vector<glm::uvec3> Mesh::GetIndecies() {
    return m_triangle_indecies;
}