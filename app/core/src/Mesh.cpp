#include "Mesh.hpp"

#include <SDL2/SDL.h>
#include <string>
#include <algorithm>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>


Mesh::Mesh(const std::filesystem::path& filename, size_t material_id, glm::mat4 transform) {
    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(filename)) {
        if (!reader.Error().empty()) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Mesh] TinyObjReader loading error: %s", reader.Error().c_str());
        }
        exit(1);
    }

    if (!reader.Warning().empty()) {
        SDL_LogMessage(SDL_LOG_CATEGORY_ERROR, SDL_LOG_PRIORITY_WARN, "[Mesh] TinyObjReader loading warning: %s", reader.Warning().c_str());
    }

    const tinyobj::attrib_t& attrib = reader.GetAttrib();
    const std::vector<tinyobj::shape_t>& shapes = reader.GetShapes();

    if (attrib.normals.size() != 0) {
        if (attrib.vertices.size() != attrib.normals.size()) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Mesh] error: obj file contains different amount of vertecies as to normals");
            exit(1);
        }
    }

    bool has_normals = (attrib.normals.size() != 0);

    std::vector<glm::vec3> normal_accumulator;

    for (size_t i = 0; i < attrib.vertices.size(); i += 3) {
        GLfloat x = static_cast<GLfloat>(attrib.vertices[i + 0]);
        GLfloat y = static_cast<GLfloat>(attrib.vertices[i + 1]);
        GLfloat z = static_cast<GLfloat>(attrib.vertices[i + 2]);

        m_vertecies.push_back(transform * glm::vec4{x, y, z, 0.0f});

        if (has_normals) {
            GLfloat nx = static_cast<GLfloat>(attrib.normals[i + 0]);
            GLfloat ny = static_cast<GLfloat>(attrib.normals[i + 1]);
            GLfloat nz = static_cast<GLfloat>(attrib.normals[i + 2]);
            m_normals.push_back(glm::vec4{nx, ny, nz, 0.0f});
        } else {
            normal_accumulator.push_back(glm::vec3{1.0f, 0.0f, 0.0f});
        }
    }

    for (const tinyobj::shape_t& shape : shapes) {
        size_t i = 0;

        for (const size_t fv : shape.mesh.num_face_vertices) {
            if (fv != 3) {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Mesh] error, only triangle faces are supported");    // tinyobjloader supposedly automatically converts all polygons to triangles if not specified otherwise
                exit(1);
            }

            tinyobj::index_t i_1 = shape.mesh.indices[i + 0];
            tinyobj::index_t i_2 = shape.mesh.indices[i + 1];
            tinyobj::index_t i_3 = shape.mesh.indices[i + 2];

            if (has_normals) {
                if (i_1.vertex_index != i_1.normal_index || i_2.vertex_index != i_2.normal_index || i_3.vertex_index != i_3.normal_index) {
                    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "[Mesh] error: different indecies for vertecies and normals are not supported");
                    exit(1);
                }
            } else {
                glm::vec3 v1 = glm::vec3{m_vertecies[i_1.vertex_index].x, m_vertecies[i_1.vertex_index].y, m_vertecies[i_1.vertex_index].z};
                glm::vec3 v2 = glm::vec3{m_vertecies[i_2.vertex_index].x, m_vertecies[i_2.vertex_index].y, m_vertecies[i_2.vertex_index].z};
                glm::vec3 v3 = glm::vec3{m_vertecies[i_3.vertex_index].x, m_vertecies[i_3.vertex_index].y, m_vertecies[i_3.vertex_index].z};

                glm::vec3 triangle_normal = glm::normalize(glm::cross((v2 - v1), (v3 - v1)));

                normal_accumulator[i_1.vertex_index] += triangle_normal;
                normal_accumulator[i_2.vertex_index] += triangle_normal;
                normal_accumulator[i_3.vertex_index] += triangle_normal;
            }

            m_triangles.push_back(glm::uvec4(
                static_cast<GLuint>(i_1.vertex_index), 
                static_cast<GLuint>(i_2.vertex_index), 
                static_cast<GLuint>(i_3.vertex_index), 
                static_cast<GLuint>(material_id)));

            i += 3;
        }
    }

    if (!has_normals) {
        for (const glm::vec3& normal : normal_accumulator) {
            m_normals.push_back(glm::vec4{glm::normalize(normal), 0.0f});
        }
    }
}

Mesh::~Mesh() {}
