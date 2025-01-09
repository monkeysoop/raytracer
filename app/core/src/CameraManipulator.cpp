#include "CameraManipulator.hpp"

#include "Camera.hpp"

#include <SDL2/SDL.h>

CameraManipulator::CameraManipulator() {}

CameraManipulator::~CameraManipulator() {}

void CameraManipulator::SetCamera(Camera* _pCamera) {
    m_pCamera = _pCamera;

    if (!m_pCamera)
        return;

    // Set the initial spherical coordinates.
    m_center = m_pCamera->GetAt();
    glm::vec3 ToAim = m_center - m_pCamera->GetEye();

    m_distance = glm::length(ToAim);

    m_u = atan2f(ToAim.z, ToAim.x);
    m_v = acosf(ToAim.y / m_distance);
}

bool CameraManipulator::Update(float _deltaTime, const Portal& portal_1, const Portal& portal_2, float portal_width, float portal_height) {
    if (!m_pCamera) {
        return false;
    }

    // Frissitjuk a kamerát a Model paraméterek alapján.

    // Az új nézési irányt a gömbi koordináták alapján számoljuk ki.
    glm::vec3 lookDirection(cosf(m_u) * sinf(m_v), cosf(m_v), sinf(m_u) * sinf(m_v));
    // Az új kamera pozíciót a nézési irány és a távolság alapján számoljuk ki.
    glm::vec3 eye = m_center - m_distance * lookDirection;

    // Az új felfelé irány a világ felfelével legyen azonos.
    glm::vec3 up = m_pCamera->GetWorldUp();

    // Az új jobbra irányt a nézési irány és a felfelé irány keresztszorzatából számoljuk ki.
    glm::vec3 right = glm::normalize(glm::cross(lookDirection, up));

    // Az új előre irányt a felfelé és jobbra irányok keresztszorzatából számoljuk ki.
    glm::vec3 forward = glm::cross(up, right);

    // Az új elmozdulásat a kamera mozgás irányának és sebességének a segítségével számoljuk ki.
    glm::vec3 deltaPosition = (m_goForward * forward + m_goRight * right + m_goUp * up) * m_speed * _deltaTime;


    glm::vec3 ray_position = m_prev_eye;
    glm::vec3 ray_direction = eye + deltaPosition - m_prev_eye;
    float distance = glm::length(ray_direction);

    bool teleported = false;
    if (distance != 0.0) {
        ray_direction = glm::normalize(ray_direction);

        float portal_1_t = portal_1.RayPortal(ray_position, ray_direction, distance, portal_width, portal_height);
        float portal_2_t = portal_2.RayPortal(ray_position, ray_direction, distance, portal_width, portal_height);

        if (portal_1_t != -1.0 && (portal_1_t < portal_2_t || portal_2_t == -1.0)) {
            glm::vec4 temp_pos = portal_1.GetDifferenceMatrixTo(portal_2) * glm::vec4{ray_position + portal_1_t * ray_direction - portal_1.GetPosition(), 1.0};
            ray_position = glm::vec3{temp_pos.x, temp_pos.y, temp_pos.z} + portal_2.GetPosition();
            
            glm::vec4 temp_dir = portal_1.GetDifferenceMatrixTo(portal_2) * glm::vec4{ray_direction, 0.0};
            ray_direction = glm::normalize(glm::vec3{temp_dir.x, temp_dir.y, temp_dir.z});
            
            ray_position += (distance - portal_1_t + 0.001f) * ray_direction;
            
            glm::vec4 temp_look_dir = portal_1.GetDifferenceMatrixTo(portal_2) * glm::vec4{lookDirection, 0.0};
            glm::vec3 new_look_dir = glm::normalize(glm::vec3{temp_look_dir.x, temp_look_dir.y, temp_look_dir.z});

            eye = ray_position;
            m_center = eye + m_distance * new_look_dir;
            
            m_u = atan2f(new_look_dir.z, new_look_dir.x);
            m_v = acosf(new_look_dir.y);
            
            teleported = true;


        } else if (portal_2_t != -1.0 && (portal_2_t < portal_1_t || portal_1_t == -1.0)) {
            glm::vec4 temp_pos = portal_2.GetDifferenceMatrixTo(portal_1) * glm::vec4{ray_position + portal_2_t * ray_direction - portal_2.GetPosition(), 1.0};
            ray_position = glm::vec3{temp_pos.x, temp_pos.y, temp_pos.z} + portal_1.GetPosition();
            
            glm::vec4 temp_dir = portal_2.GetDifferenceMatrixTo(portal_1) * glm::vec4{ray_direction, 0.0};
            ray_direction = glm::normalize(glm::vec3{temp_dir.x, temp_dir.y, temp_dir.z});
            
            ray_position += (distance - portal_2_t + 0.001f) * ray_direction;
            
            glm::vec4 temp_look_dir = portal_2.GetDifferenceMatrixTo(portal_1) * glm::vec4{lookDirection, 0.0};
            glm::vec3 new_look_dir = glm::normalize(glm::vec3{temp_look_dir.x, temp_look_dir.y, temp_look_dir.z});

            eye = ray_position;
            m_center = eye + m_distance * new_look_dir;
            
            m_u = atan2f(new_look_dir.z, new_look_dir.x);
            m_v = acosf(new_look_dir.y);
            
            teleported = true;
        }
    }

    if (!teleported) {
        // Az új kamera pozíciót és nézési cél pozíciót beállítjuk.
        eye += deltaPosition;
        m_center += deltaPosition;
    }
    
    glm::vec3 world_up = m_pCamera->GetWorldUp();
    if (eye != m_prev_eye || m_center != m_prev_center || world_up != m_prev_world_up) {
        m_prev_eye = eye;
        m_prev_center = m_center ;
        m_prev_world_up = world_up;
        
        // Frissítjük a kamerát az új pozícióval és nézési iránnyal.
        m_pCamera->SetView(eye, m_center, world_up);
        return true;
    } else {
        return false;
    }
}

void CameraManipulator::KeyboardDown(const SDL_KeyboardEvent& key) {
    switch (key.keysym.sym) {
        case SDLK_LSHIFT:
        case SDLK_RSHIFT:
            if (key.repeat == 0)
                m_speed /= 4.0f;
            break;
        case SDLK_w:
            m_forward_pressed = true;
            m_goForward = 1;
            break;
        case SDLK_s:
            m_backward_pressed = true;
            m_goForward = -1;
            break;
        case SDLK_a:
            m_left_pressed = true;
            m_goRight = -1;
            break;
        case SDLK_d:
            m_right_pressed = true;
            m_goRight = 1;
            break;
        case SDLK_e:
            m_up_pressed = true;
            m_goUp = 1;
            break;
        case SDLK_q:
            m_down_pressed = true;
            m_goUp = -1;
            break;
    }
}

void CameraManipulator::KeyboardUp(const SDL_KeyboardEvent& key) {
    switch (key.keysym.sym) {
        case SDLK_LSHIFT:
        case SDLK_RSHIFT:
            m_speed *= 4.0f;
            break;
        case SDLK_w:
            m_forward_pressed = false;
            m_goForward = m_backward_pressed ? -1 : 0;
            break;
        case SDLK_s:
            m_backward_pressed = false;
            m_goForward = m_forward_pressed ? 1 : 0;
            break;
        case SDLK_a:
            m_left_pressed = false;
            m_goRight = m_right_pressed ? 1 : 0;
            break;
        case SDLK_d:
            m_right_pressed = false;
            m_goRight = m_left_pressed ? -1 : 0;
            break;
        case SDLK_q:
            m_down_pressed = false;
            m_goUp = m_up_pressed ? 1 : 0;
            break;
        case SDLK_e:
            m_up_pressed = false;
            m_goUp = m_down_pressed ? -1 : 0;
            break;
    }
}

void CameraManipulator::MouseMove(const SDL_MouseMotionEvent& mouse) {
    if (mouse.state & SDL_BUTTON_LMASK) {
        float du = mouse.xrel / 100.0f;
        float dv = mouse.yrel / 100.0f;

        m_u += du;
        m_v = glm::clamp<float>(m_v + dv, 0.1f, 3.1f);
    }
    if (mouse.state & SDL_BUTTON_RMASK) {
        float dDistance = mouse.yrel / 100.0f;
        m_distance += dDistance;
    }
}

void CameraManipulator::MouseWheel(const SDL_MouseWheelEvent& wheel) {
    float dDistance = static_cast<float>(wheel.y) * m_speed / -100.0f;
    m_distance += dDistance;
}