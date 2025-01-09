#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

class Portal {
public:
    Portal(glm::vec3 position, glm::vec3 direction);
    ~Portal();

    glm::vec3 GetPosition() const;
    glm::vec3 GetDirection() const;

    glm::mat4 GetDifferenceMatrixTo(const Portal& destination_portal) const;

    float RayPortal(glm::vec3 ray_position, glm::vec3 ray_direction, float closest_distance, float portal_width, float portal_height) const;

private:
    glm::vec3 m_position;
    glm::vec3 m_direction;
};