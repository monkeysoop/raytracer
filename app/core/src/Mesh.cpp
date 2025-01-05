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
        exit(1);
        return;
    }

    if (!reader.Warning().empty()) {
        SDL_LogMessage(SDL_LOG_CATEGORY_ERROR, SDL_LOG_PRIORITY_WARN, "[Mesh] TinyObjReader loading warning: %s", reader.Warning().c_str());
    }

    const tinyobj::attrib_t& attrib = reader.GetAttrib();
    const std::vector<tinyobj::shape_t>& shapes = reader.GetShapes();

    if (attrib.vertices.size() != attrib.normals.size()) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Mesh] error: obj file doesn't contain vertex normals");
        exit(1);
    }

    for (size_t i = 0; i < attrib.vertices.size(); i += 3) {
        GLfloat x = static_cast<GLfloat>(attrib.vertices[i + 0]);
        GLfloat y = static_cast<GLfloat>(attrib.vertices[i + 1]);
        GLfloat z = static_cast<GLfloat>(attrib.vertices[i + 2]);

        GLfloat nx = static_cast<GLfloat>(attrib.normals[i + 0]);
        GLfloat ny = static_cast<GLfloat>(attrib.normals[i + 1]);
        GLfloat nz = static_cast<GLfloat>(attrib.normals[i + 2]);

        m_vertecies.push_back(glm::vec4{x, y, z, 0.0f});
        m_normals.push_back(glm::vec4{nx, ny, nz, 0.0f});
    }

    for (const tinyobj::shape_t& shape : shapes) {
        size_t i = 0;

        for (const size_t fv : shape.mesh.num_face_vertices) {
            if (fv != 3) {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Mesh] error, only triangle faces are supported");    // tinyobjloader supposedly automatically converts all polygons to triangles if not specified otherwise
                return;
                exit(1);
            }

            tinyobj::index_t i_1 = shape.mesh.indices[i + 0];
            tinyobj::index_t i_2 = shape.mesh.indices[i + 1];
            tinyobj::index_t i_3 = shape.mesh.indices[i + 2];

            if (i_1.vertex_index != i_1.normal_index
             || i_2.vertex_index != i_2.normal_index
             || i_3.vertex_index != i_3.normal_index) {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Mesh] error: different indecies for vertecies and normals are not supported");
                exit(1);
                return;
            }

            GLuint index_1 = static_cast<GLuint>(i_1.vertex_index);
            GLuint index_2 = static_cast<GLuint>(i_2.vertex_index);
            GLuint index_3 = static_cast<GLuint>(i_3.vertex_index);

            m_triangle_indecies.push_back(glm::uvec3(index_1, index_2, index_3));

            i += 3;
        }
    }
}

Mesh::~Mesh() {}
