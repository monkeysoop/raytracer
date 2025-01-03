#pragma once

#include <GL/glew.h>
#include <vector>
#include <array>
#include <memory>

#include "Mesh.hpp"

struct AABB {
    glm::vec3 min_bounds;
    glm::vec3 max_bounds;
};

struct OctreeNode {
    struct AABB bounding_box;
    std::vector<glm::uvec3> triangles;
    std::array<std::unique_ptr<OctreeNode>, 8> childrens;
    bool is_leaf;
};

class Octree {
public:
    Octree(std::vector<Mesh> meshes, size_t depth_limit, size_t max_triangles_per_node);
    ~Octree();

private:
    void Traverse(std::unique_ptr<OctreeNode>& node, size_t current_depth);
    void Subdivide(std::unique_ptr<OctreeNode>& node, size_t current_depth);


    std::vector<glm::vec3> m_vertecies;
    std::unique_ptr<OctreeNode> m_root;
    size_t m_max_depth;
    const size_t m_depth_limit;
    const size_t m_max_triangles_per_node; // a leaf could have more triangles than this limit if it is at the depth limit
};