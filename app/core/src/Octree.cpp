#include "Octree.hpp"

#include "TriangleBoxIntersection.hpp"

#include <limits>
#include <numeric>
#include <algorithm>
#include <string>
#include <cmath>


#include <iostream>

float Average(std::vector<size_t> vector) {
    return vector.empty() ? 0.0 : (std::accumulate(vector.begin(), vector.end(), 0.0) / vector.size());
}

bool AABBTriangleOverlapTest(AABB aabb, glm::vec3 v1, glm::vec3 v2, glm::vec3 v3) {
    glm::vec3 box_center{(aabb.min_bounds + aabb.max_bounds) / 2.0f};
    glm::vec3 box_half_size{(aabb.max_bounds - aabb.min_bounds) / 2.0f};
 
    return triBoxOverlap(box_center, box_half_size, v1, v2, v3);
}

std::string SizeToString(size_t size_in_bytes) {
    if (size_in_bytes < 1024) {
        return std::to_string(size_in_bytes) + " B"; 
    } else if (size_in_bytes < 1024 * 1024) {
        return std::to_string(size_in_bytes / 1024) + " KB"; 
    } else if (size_in_bytes < 1024 * 1024 * 1024) {
        return std::to_string(size_in_bytes / (1024 * 1024)) + " MB"; 
    } else {
        return std::to_string(size_in_bytes / (1024 * 1024 * 1024)) + " GB"; 
    }

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
            if (!node->childrens[i]->is_leaf || node->childrens[i]->triangles.size() != 0) {
                children_mask |= (0x01 << i);
                children_count++;
            }
        }
    }
    
    uint32_t node_info = (triangle_count << 16) | (children_mask << 8) | children_count;
    uint32_t triangle_start = static_cast<uint32_t>(m_compressed_triangles.size());

    m_compressed_triangles.insert(m_compressed_triangles.end(), node->triangles.cbegin(), node->triangles.cend());
    //for (const glm::uvec4& ind : node->triangles) {
    //    m_compressed_triangles.push_back(static_cast<uint32_t>(ind.x));
    //    m_compressed_triangles.push_back(static_cast<uint32_t>(ind.y));
    //    m_compressed_triangles.push_back(static_cast<uint32_t>(ind.z));
    //}
    
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

void Octree::DepthFirstTraverse(
    std::unique_ptr<OctreeNode>& node, 
    size_t current_depth, 
    std::vector<std::vector<size_t>>& triangles_per_level, 
    std::vector<std::vector<size_t>>& children_count_per_level, 
    std::vector<size_t>& leaf_depths
) {
    //for (size_t i = 0; i < current_depth; i++) {
    //    std::cout << '\t';
    //}

    triangles_per_level[current_depth].push_back(node->triangles.size());

    //std::cout << "size: " << node->triangles.size();
    //std::cout << (node->is_leaf ? " leaf" : " node") << " depth: " << current_depth << std::endl;

    if (!node->is_leaf) {
        size_t children_count = 0;
        for (size_t i = 0; i < 8; i++) {
            if (!node->childrens[i]->is_leaf || node->childrens[i]->triangles.size() != 0) {
                children_count++;
                DepthFirstTraverse(node->childrens[i], current_depth + 1, triangles_per_level, children_count_per_level, leaf_depths);
            }
        }
        children_count_per_level[current_depth].push_back(children_count);
    } else {
        leaf_depths.push_back(current_depth);
    }

}

void Octree::Subdivide(std::unique_ptr<OctreeNode>& node, size_t current_depth) {
    if (current_depth >= m_depth_limit) {
        return;
    } else if (node->triangles.size() <= m_max_triangles_per_leaf) {
        m_max_depth = std::max(m_max_depth, current_depth);
        return;
    }
    m_max_depth = std::max(m_max_depth, current_depth);

    
    node->is_leaf = false;

    glm::vec3 mid_point{(node->bounding_box.min_bounds + node->bounding_box.max_bounds) / 2.0f};

    for (size_t i = 0; i < 8; i++) {
        glm::vec3 children_min_bound{
            (i & 1) ? mid_point.x : node->bounding_box.min_bounds.x,
            (i & 2) ? mid_point.y : node->bounding_box.min_bounds.y,
            (i & 4) ? mid_point.z : node->bounding_box.min_bounds.z
        };
        glm::vec3 children_max_bound{
            (i & 1) ? node->bounding_box.max_bounds.x : mid_point.x,
            (i & 2) ? node->bounding_box.max_bounds.y : mid_point.y,
            (i & 4) ? node->bounding_box.max_bounds.z : mid_point.z
        };

        node->childrens[i] = std::make_unique<OctreeNode>(OctreeNode{AABB{children_min_bound, children_max_bound}, {}, {}, true});
    }

    std::vector<glm::uvec4> kept_triangle_indecies{};

    std::vector<std::pair<std::vector<size_t>, glm::uvec4>> childrens_overlappings;

    for (const glm::uvec4& ind : node->triangles) {
        glm::vec3 v1{m_vertecies[ind.x].x, m_vertecies[ind.x].y, m_vertecies[ind.x].z};
        glm::vec3 v2{m_vertecies[ind.y].x, m_vertecies[ind.y].y, m_vertecies[ind.y].z};
        glm::vec3 v3{m_vertecies[ind.z].x, m_vertecies[ind.z].y, m_vertecies[ind.z].z};

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
    }

    if (childrens_overlappings.size() > m_max_triangles_per_node) {
        std::partial_sort(
            childrens_overlappings.begin(), 
            childrens_overlappings.begin() + m_max_triangles_per_node, 
            childrens_overlappings.end(),
            [](const std::pair<std::vector<size_t>, glm::uvec4>& a, const std::pair<std::vector<size_t>, glm::uvec4>& b) {
                return a.first.size() > b.first.size(); // sorts in descending order
            } 
        );
    } 
    for (size_t i = 0; i < std::min(childrens_overlappings.size(), m_max_triangles_per_node); i++) {
        kept_triangle_indecies.push_back(childrens_overlappings[i].second);
    }
    
    for (size_t i = m_max_triangles_per_node; i < childrens_overlappings.size(); i++) {
        for (size_t child_index : childrens_overlappings[i].first) {
            node->childrens[child_index]->triangles.push_back(childrens_overlappings[i].second);
        }
    }

    node->triangles = std::move(kept_triangle_indecies);

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
    std::vector<glm::vec4> combined_vertecies{};
    std::vector<glm::vec4> combined_normals{};
    std::vector<glm::uvec4> combined_triangles{};

    combined_vertecies.reserve(std::accumulate(meshes.begin(), meshes.end(), 0, [](size_t sum, const Mesh& mesh) {return sum + mesh.m_vertecies.size();}));
    combined_normals.reserve(std::accumulate(meshes.begin(), meshes.end(), 0, [](size_t sum, const Mesh& mesh) {return sum + mesh.m_normals.size();}));
    combined_triangles.reserve(std::accumulate(meshes.begin(), meshes.end(), 0, [](size_t sum, const Mesh& mesh) {return sum + mesh.m_triangles.size();}));

    glm::vec3 min_bounds{std::numeric_limits<float>::infinity()};
    glm::vec3 max_bounds{-1.0f * std::numeric_limits<float>::infinity()};

    for (const Mesh& mesh : meshes) {
        GLuint prev_size = static_cast<GLuint>(combined_vertecies.size());

        combined_vertecies.insert(combined_vertecies.end(), mesh.m_vertecies.cbegin(), mesh.m_vertecies.cend());
        combined_normals.insert(combined_normals.end(), mesh.m_normals.cbegin(), mesh.m_normals.cend());
        std::transform(
            mesh.m_triangles.cbegin(), 
            mesh.m_triangles.cend(), 
            std::back_inserter(combined_triangles), 
            [prev_size](glm::uvec4 ind) {
                return ind + glm::uvec4{prev_size, prev_size, prev_size, 0};
            });

        for (const glm::uvec4& ind : mesh.m_triangles) {
            glm::vec3 v1{mesh.m_vertecies[ind.x].x, mesh.m_vertecies[ind.x].y, mesh.m_vertecies[ind.x].z};
            glm::vec3 v2{mesh.m_vertecies[ind.y].x, mesh.m_vertecies[ind.y].y, mesh.m_vertecies[ind.y].z};
            glm::vec3 v3{mesh.m_vertecies[ind.z].x, mesh.m_vertecies[ind.z].y, mesh.m_vertecies[ind.z].z};

            min_bounds = glm::min(min_bounds, v1);
            min_bounds = glm::min(min_bounds, v2);
            min_bounds = glm::min(min_bounds, v3);

            max_bounds = glm::max(max_bounds, v1);
            max_bounds = glm::max(max_bounds, v2);
            max_bounds = glm::max(max_bounds, v3);
        }
    }

    m_vertecies = std::move(combined_vertecies);
    m_normals = std::move(combined_normals);

    m_root = std::make_unique<OctreeNode>(OctreeNode{AABB{min_bounds, max_bounds}, combined_triangles, {}, true});
    
    Subdivide(m_root, 1);
    DepthFirstCompress(m_root, 0);


    std::vector<std::vector<size_t>> triangles_per_level;
    std::vector<std::vector<size_t>> children_count_per_level;
    std::vector<size_t> leaf_depths;

    for (size_t i = 0; i < (m_max_depth + 1); i++) {
        triangles_per_level.push_back(std::vector<size_t>{});
        children_count_per_level.push_back(std::vector<size_t>{});
    }

    DepthFirstTraverse(m_root, 0, triangles_per_level, children_count_per_level, leaf_depths);

    
    
    std::cout << "............................................................." << std::endl;
    for (size_t i = 0; i < triangles_per_level.size(); i++) {
        std::cout << "level: " << i << " avg triangles per level: " << Average(triangles_per_level[i]) << std::endl;
    }
    for (size_t i = 0; i < children_count_per_level.size(); i++) {
        std::cout << "level: " << i << " avg children count per level: " << Average(children_count_per_level[i]) << std::endl;
    }
    std::cout << "max depth: " << m_max_depth << std::endl;
    std::cout << "average leaf depth: " << Average(leaf_depths) << std::endl;
    std::cout << "............................................................." << std::endl;
    std::cout << "vertecies count: " << m_vertecies.size() << std::endl;
    std::cout << "triangle count: " << combined_triangles.size() << std::endl;
    std::cout << "vertecies size: " << SizeToString(m_vertecies.size() * 16) << std::endl;
    std::cout << "normals size: " << SizeToString(m_normals.size() * 16) << std::endl;
    std::cout << "uncompressed triangle size: " << SizeToString(combined_triangles.size() * 16) << std::endl;
    std::cout << "............................................................." << std::endl;
    std::cout << "compressed node size: " << SizeToString(m_compressed_node_buffer.size() * 4) << std::endl;
    std::cout << "compresed triangle size: " << SizeToString(m_compressed_triangles.size() * 4) << std::endl;
    std::cout << "............................................................." << std::endl;

}

Octree::~Octree() {}

glm::vec3 Octree::GetMinBounds() {
    return m_root->bounding_box.min_bounds;
}

glm::vec3 Octree::GetMaxBounds() {
    return m_root->bounding_box.max_bounds;
}