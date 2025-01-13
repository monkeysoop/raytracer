#include "Portal.hpp"

Portal::Portal(glm::vec3 position, glm::vec3 direction) : m_position{position}, m_direction{direction} {}

Portal::~Portal() {}

glm::vec3 Portal::GetPosition() const {
    return m_position;
}

glm::vec3 Portal::GetDirection() const {
    return m_direction;
}

glm::mat4 Portal::GetDifferenceMatrixTo(const Portal& destination_portal) const {
    glm::vec3 dest_direction = destination_portal.GetDirection();

    float angle = glm::acos(glm::dot(m_direction, dest_direction));
    glm::vec3 axis = glm::cross(m_direction, dest_direction);

    if (glm::length(axis) > 0.0) {
        return glm::rotate(angle, glm::normalize(axis));
    } else {
        return glm::identity<glm::mat4>();
    }
}

float Portal::RayPortal(glm::vec3 ray_position, glm::vec3 ray_direction, float closest_distance, float portal_width, float portal_height) const {
    float d = glm::dot(m_direction, ray_direction);
    
    if (glm::abs(d) <= 0.0001) {
        return -1.0;
    }

    float t = glm::dot((m_position - ray_position), m_direction) / d;
    
    if (t < 0.0 || t > closest_distance) {
        return -1.0;
    }

    glm::vec3 intersect_point = ray_position + t * ray_direction;

    glm::vec3 plane_right = glm::cross(m_direction, glm::vec3(0.0, 1.0, 0.0));
    if (length(plane_right) <= 0.0001) {
        return -1.0;
    }
    
    plane_right = glm::normalize(plane_right);
    glm::vec3 plane_up = glm::normalize(glm::cross(plane_right, m_direction));

    glm::vec3 c = intersect_point - m_position;

    if (glm::abs(glm::dot(plane_right, c)) < 0.5 * portal_width && glm::abs(glm::dot(plane_up, c)) < 0.5 * portal_height) {
        return t;
    } else {
        return -1.0;
    }
}