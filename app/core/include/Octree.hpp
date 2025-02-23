#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <vector>
#include <array>
#include <memory>
#include <cstdint>

#include "Mesh.hpp"

struct AABB {
    glm::vec3 min_bounds;
    glm::vec3 max_bounds;
};

struct OctreeNode {
    struct AABB bounding_box;
    std::vector<glm::uvec4> triangles;
    std::array<std::unique_ptr<OctreeNode>, 8> childrens;
    bool is_leaf;
};

class Octree {
public:
    Octree(std::vector<Mesh> meshes, size_t depth_limit, size_t max_triangles_per_node, size_t max_triangles_per_leaf, size_t m_keep_triangles_after_this_many_overlaps);
    ~Octree();

    glm::vec3 GetMinBounds();
    glm::vec3 GetMaxBounds();

    std::vector<glm::vec4> m_vertecies;
    std::vector<glm::vec4> m_normals;
    std::vector<uint32_t> m_compressed_node_buffer;
    std::vector<glm::uvec4> m_compressed_triangles;

private:
    void DepthFirstCompress(std::unique_ptr<OctreeNode>& node, size_t parent_node_child_pointer_location);
    void Subdivide(std::unique_ptr<OctreeNode>& node, size_t current_depth);
    void DepthFirstTraverse(
        std::unique_ptr<OctreeNode>& node, 
        size_t current_depth, 
        std::vector<std::vector<size_t>>& triangles_per_level, 
        std::vector<std::vector<size_t>>& children_count_per_level, 
        std::vector<size_t>& leaf_depths
    );

    std::unique_ptr<OctreeNode> m_root;
    size_t m_max_depth;

    const size_t m_depth_limit; // inclusive
    const size_t m_max_triangles_per_node; // it shouldn't have more than 65536 (2^16)
    const size_t m_max_triangles_per_leaf; // a leaf could have more triangles than this limit if it is at the depth limit, it shouldn't have more than 65536 (2^16)
    const size_t m_keep_triangles_after_this_many_overlaps; // if lets say this is set at 5 and a triangle intersects at least 5 childrens aabb than that triangle won't be copied into the childrens rather it will be kept in the node 
};