#include "Octree.hpp"

#include "TriangleBoxIntersection.hpp"

#include <limits>
#include <numeric>
#include <algorithm>


#include <iostream>
#include <stdio.h>

bool AABBTriangleOverlapTest(AABB aabb, glm::vec3 v1, glm::vec3 v2, glm::vec3 v3) {
    glm::vec3 box_center{(aabb.min_bounds + aabb.max_bounds) / 2.0f};
    glm::vec3 box_half_size{(aabb.max_bounds - aabb.min_bounds) / 2.0f};
 
    return triBoxOverlap(box_center, box_half_size, v1, v2, v3);
}

void Octree::DepthFirstCompress(std::unique_ptr<OctreeNode>& node, size_t parent_node_child_pointer_location) {
    if (parent_node_child_pointer_location != 0) {
        uint32_t node_start = static_cast<uint32_t>(m_compressed_node_buffer.size());
        m_compressed_node_buffer[parent_node_child_pointer_location] = node_start;
    }

    uint16_t triangle_count = static_cast<uint16_t>(node->triangles.size());
    uint8_t children_mask = 0x00;
    uint8_t children_count = 0; 
    
    if (!node->is_leaf) {
        for (size_t i = 0; i < 8; i++) {
            if (!node->childrens[i]->is_leaf) {
                children_mask |= (0x01 << i);
                children_count++;
            }
        }
    }
    
    uint32_t node_info = (triangle_count << 16) | (children_mask << 8) | children_count;
    uint32_t triangle_start = static_cast<uint32_t>(m_compressed_triangle_indecies.size());

    //printf("node: 0x%04x  0x%02x  0x%02x  %d\n", triangle_count, children_mask, children_count, node->is_leaf);

    for (const glm::uvec3& ind : node->triangles) {
        m_compressed_triangle_indecies.push_back(static_cast<uint32_t>(ind.x));
        m_compressed_triangle_indecies.push_back(static_cast<uint32_t>(ind.y));
        m_compressed_triangle_indecies.push_back(static_cast<uint32_t>(ind.z));
    }
    
    m_compressed_node_buffer.push_back(node_info);
    m_compressed_node_buffer.push_back(triangle_start);

    size_t child_pointers_start = m_compressed_node_buffer.size();
    
    for (size_t i = 0; i < children_count; i++) {
        uint32_t placeholder = 0; // when the child node is called it will overwrite this
        m_compressed_node_buffer.push_back(placeholder);
    }

    size_t child_pointer_offset = 0;
    for (size_t i = 0; i < 8; i++) {
        if (children_mask & (0x01 << i)) {
            DepthFirstCompress(node->childrens[i], (child_pointers_start + child_pointer_offset));
            child_pointer_offset++;
        }
    }

}

void Octree::DepthFirstTraverse(std::unique_ptr<OctreeNode>& node, size_t current_depth) {
    for (size_t i = 0; i < current_depth; i++) {
        std::cout << '\t';
    }
    std::cout << "size: " << node->triangles.size();

    std::cout << (node->is_leaf ? " leaf" : " node") << " depth: " << current_depth << std::endl;

    if (!node->is_leaf) {
        for (size_t i = 0; i < 8; i++) {
            if (!node->childrens[i]->is_leaf || node->childrens[i]->triangles.size() != 0) {
                //std::cout << node->childrens[i]->triangles.size() << std::endl;
                DepthFirstTraverse(node->childrens[i], current_depth + 1);
            }
        }
    }
}

void Octree::Subdivide(std::unique_ptr<OctreeNode>& node, size_t current_depth) {
    //std::cout << "depth: " << current_depth << std::endl;

    if (current_depth >= m_depth_limit) {
        return;
    } else if (node->triangles.size() <= m_max_triangles_per_leaf) {
        m_max_depth = std::max(m_max_depth, current_depth);
        return;
    }
    m_max_depth = std::max(m_max_depth, current_depth);

    
    node->is_leaf = false;

    glm::vec3 mid_point{(node->bounding_box.min_bounds + node->bounding_box.max_bounds) / 2.0f};

    std::array<glm::vec3, 8> children_min_bounds{
        glm::vec3(node->bounding_box.min_bounds.x, node->bounding_box.min_bounds.y, node->bounding_box.min_bounds.z),
        glm::vec3(mid_point.x,                     node->bounding_box.min_bounds.y, node->bounding_box.min_bounds.z),
        glm::vec3(node->bounding_box.min_bounds.x, mid_point.y,                     node->bounding_box.min_bounds.z),
        glm::vec3(mid_point.x,                     mid_point.y,                     node->bounding_box.min_bounds.z),
        glm::vec3(node->bounding_box.min_bounds.x, node->bounding_box.min_bounds.y, mid_point.z),
        glm::vec3(mid_point.x,                     node->bounding_box.min_bounds.y, mid_point.z),
        glm::vec3(node->bounding_box.min_bounds.x, mid_point.y,                     mid_point.z),
        glm::vec3(mid_point.x,                     mid_point.y,                     mid_point.z)
    };

    std::array<glm::vec3, 8> children_max_bounds{
        glm::vec3(mid_point.x,                     mid_point.y,                     mid_point.z),
        glm::vec3(node->bounding_box.max_bounds.x, mid_point.y,                     mid_point.z),
        glm::vec3(mid_point.x,                     node->bounding_box.max_bounds.y, mid_point.z),
        glm::vec3(node->bounding_box.max_bounds.x, node->bounding_box.max_bounds.y, mid_point.z),
        glm::vec3(mid_point.x,                     mid_point.y,                     node->bounding_box.max_bounds.z),
        glm::vec3(node->bounding_box.max_bounds.x, mid_point.y,                     node->bounding_box.max_bounds.z),
        glm::vec3(mid_point.x,                     node->bounding_box.max_bounds.y, node->bounding_box.max_bounds.z),
        glm::vec3(node->bounding_box.max_bounds.x, node->bounding_box.max_bounds.y, node->bounding_box.max_bounds.z)
    };

    for (size_t i = 0; i < 8; i++) {
        node->childrens[i] = std::make_unique<OctreeNode>(OctreeNode{AABB{children_min_bounds[i], children_max_bounds[i]}, {}, {}, true});
    }

    std::vector<glm::uvec3> kept_triangle_indecies{};

    //std::cout << node->triangles.size() << std::endl;


    std::vector<std::pair<std::vector<size_t>, glm::uvec3>> childrens_overlappings;

    for (const glm::uvec3& ind : node->triangles) {
        glm::vec3 v1 = m_vertecies[ind.x];
        glm::vec3 v2 = m_vertecies[ind.y];
        glm::vec3 v3 = m_vertecies[ind.z];

        std::vector<size_t> childrens_overlap{};
        for (size_t child_index = 0; child_index < 8; child_index++) {
            if (AABBTriangleOverlapTest(node->childrens[child_index]->bounding_box, v1, v2, v3)) {
                childrens_overlap.push_back(child_index);
            }
        }

        if (childrens_overlap.size() < m_keep_triangles_after_this_many_overlaps) {
            for (size_t child_index : childrens_overlap) {
                node->childrens[child_index]->triangles.push_back(ind);
            }
        } else  {
            childrens_overlappings.push_back({childrens_overlap, ind});
        }

        //std::cout << overlapped_childrens.size() << std::endl;
    }

    //std::cout << "\tsize: " << childrens_overlappings.size() << std::endl;

    if (childrens_overlappings.size() > m_max_triangles_per_node) {
        //for (auto i : childrens_overlappings) {
        //    std::cout << i.first.size() << " ";
        //}
        //std::cout << std::endl;
        std::partial_sort(
            childrens_overlappings.begin(), 
            childrens_overlappings.begin() + m_max_triangles_per_node, 
            childrens_overlappings.end(),
            [](const std::pair<std::vector<size_t>, glm::uvec3>& a, const std::pair<std::vector<size_t>, glm::uvec3>& b) {
                return a.first.size() > b.first.size(); // sorts in descending order
            } 
        );
        //for (auto i : childrens_overlappings) {
        //    std::cout << i.first.size() << " ";
        //}
        //std::cout << std::endl;

        //for (size_t i = 0; i < m_max_triangles_per_node; i++) {
        //    kept_triangle_indecies.push_back(childrens_overlappings[i].second);
        //}
        //
        //for (size_t i = m_max_triangles_per_node; i < childrens_overlappings.size(); i++) {
        //    for (size_t child_index : childrens_overlappings[i].first) {
        //        node->childrens[child_index]->triangles.push_back(childrens_overlappings[i].second);
        //    }
        //}
    } 
    for (size_t i = 0; i < std::min(childrens_overlappings.size(), m_max_triangles_per_node); i++) {
        kept_triangle_indecies.push_back(childrens_overlappings[i].second);
    }
    
    for (size_t i = m_max_triangles_per_node; i < childrens_overlappings.size(); i++) {
        for (size_t child_index : childrens_overlappings[i].first) {
            node->childrens[child_index]->triangles.push_back(childrens_overlappings[i].second);
        }
    }
    //else {
    //    for (size_t i = 0; i < childrens_overlappings.size(); i++) {
    //        kept_triangle_indecies.push_back(childrens_overlappings[i].second);
    //    }
    //}

    //std::cout << "\tsize: " << kept_triangle_indecies.size() << std::endl;


    node->triangles.swap(kept_triangle_indecies);

    //std::cout << "kept size: " << kept_triangle_indecies.size() << std::endl;

    //for (size_t i = 0; i < 8; i++) {
    //    std::cout << "children: " << node->childrens[i]->triangles.size() << std::endl;
    //}
    //std::cout << std::endl;

    //node->triangles.swap(kept_triangle_indecies);

    for (size_t i = 0; i < 8; i++) {
        if (node->childrens[i]->triangles.size() != 0) {
            Subdivide(node->childrens[i], current_depth + 1);
        }
    }


}

Octree::Octree(std::vector<Mesh> meshes, size_t depth_limit, size_t max_triangles_per_node, size_t max_triangles_per_leaf, size_t m_keep_triangles_after_this_many_overlaps) : 
    m_max_depth{0}, 
    m_depth_limit{depth_limit}, 
    m_max_triangles_per_node{max_triangles_per_node},
    m_max_triangles_per_leaf{max_triangles_per_leaf}, 
    m_keep_triangles_after_this_many_overlaps{m_keep_triangles_after_this_many_overlaps}
{
    std::vector<glm::vec3> combined_vertecies{};
    std::vector<glm::uvec3> combined_triangle_indecies{};

    combined_vertecies.reserve(std::accumulate(meshes.begin(), meshes.end(), 0, [](size_t sum, const Mesh& mesh) {return sum + mesh.m_vertecies.size();}));
    combined_triangle_indecies.reserve(std::accumulate(meshes.begin(), meshes.end(), 0, [](size_t sum, const Mesh& mesh) {return sum + mesh.m_triangle_indecies.size();}));

    glm::vec3 min_bounds{std::numeric_limits<float>::infinity()};
    glm::vec3 max_bounds{-1.0f * std::numeric_limits<float>::infinity()};

    for (const Mesh& mesh : meshes) {
        combined_vertecies.insert(combined_vertecies.end(), mesh.m_vertecies.cbegin(), mesh.m_vertecies.cend());

        GLuint prev_size = static_cast<GLuint>(combined_triangle_indecies.size());
        std::transform(mesh.m_triangle_indecies.cbegin(), mesh.m_triangle_indecies.cend(), std::back_inserter(combined_triangle_indecies), [prev_size](glm::uvec3 ind) {return ind + glm::uvec3{prev_size};});

        for (const glm::uvec3& ind : mesh.m_triangle_indecies) {
            glm::vec3 v1 = mesh.m_vertecies[ind.x];
            glm::vec3 v2 = mesh.m_vertecies[ind.y];
            glm::vec3 v3 = mesh.m_vertecies[ind.z];

            min_bounds = glm::min(min_bounds, v1);
            min_bounds = glm::min(min_bounds, v2);
            min_bounds = glm::min(min_bounds, v3);

            max_bounds = glm::max(max_bounds, v1);
            max_bounds = glm::max(max_bounds, v2);
            max_bounds = glm::max(max_bounds, v3);
        }
    }

    m_vertecies = std::move(combined_vertecies);

    m_root = std::make_unique<OctreeNode>(OctreeNode{AABB{min_bounds, max_bounds}, combined_triangle_indecies, {}, true});
    
    Subdivide(m_root, 1);

    std::cout << "............................................................." << std::endl;

    DepthFirstTraverse(m_root, 0);

    DepthFirstCompress(m_root, 0);
    
    std::cout << "compressed node size: " << m_compressed_node_buffer.size() << std::endl;
    std::cout << "compresed triangle size: " << m_compressed_triangle_indecies.size() << std::endl;
    std::cout << "uncompressed triangle size: " << combined_triangle_indecies.size() << std::endl;
    std::cout << "max depth: " << m_max_depth << std::endl;
}

Octree::~Octree() {}
