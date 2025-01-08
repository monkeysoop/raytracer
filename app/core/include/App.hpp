#pragma once

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

// GLEW
#include <GL/glew.h>

// SDL
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include "Buffer.hpp"
#include "Camera.hpp"
#include "CameraManipulator.hpp"
#include "Framebuffer.hpp"
#include "Mesh.hpp"
#include "Octree.hpp"
#include "Portal.hpp"
#include "Shader.hpp"
#include "Skybox.hpp"


class App {
public:
    App(GLsizei width, GLsizei height);
    ~App();

    void Update(float elapsed_time_in_seconds, float delta_time_in_seconds);
    void Render();
    void RenderImGui();

    void KeyboardDown(const SDL_KeyboardEvent& key);
    void KeyboardUp(const SDL_KeyboardEvent& key);
    void MouseMove(const SDL_MouseMotionEvent& mouse);
    void MouseWheel(const SDL_MouseWheelEvent& wheel);
    void Resize(GLsizei width, GLsizei height);

private:
    GLsizei m_width;
    GLsizei m_height;

    Camera m_camera;
    CameraManipulator m_camera_manipulator;
    Framebuffer m_framebuffer;
    Shader m_ray_tracer_shader;

    Mesh m_mesh_1;
    Mesh m_mesh_2;
    Mesh m_mesh_3;

    Octree m_octree;

    Buffer m_vertecies_buffer;
    Buffer m_normal_buffer;
    Buffer m_indecies_buffer;
    Buffer m_node_buffer;

    Skybox m_skybox;

    GLuint m_empty_vao; 

    float m_time_in_seconds;
    size_t m_still_frame_counter;

    Portal m_portal_1;
    Portal m_portal_2;

    float m_portal_width;
    float m_portal_height;
};