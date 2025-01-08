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

    glm::mat4 GetDifferenceMatrixTo(const Portal& destination_portal);

private:
    glm::vec3 m_position;
    glm::vec3 m_direction;
};