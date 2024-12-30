#include <imgui.h>

#include "App.hpp"
#include "SDL_GLDebugMessageCallback.h"

#include <iostream>


App::App(GLsizei width, GLsizei height) : 
    m_width{width}, 
    m_height{height}, 
    m_camera{}, 
    m_camera_manipulator{}, 
    //m_framebuffer{width, height}, 
    m_ray_tracer_shader{"assets/ray_tracer.vert", "assets/ray_tracer.frag"},
    //m_mesh{"assets/suzanne.obj"},
    m_mesh{"assets/stanford_bunny.obj"},
    //m_mesh{"assets/xyzrgb_dragon.obj"},
    m_skybox{},
    m_time_in_seconds{0.0f}
{
    GLint context_flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
    if (context_flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
        glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR, GL_DONT_CARE, 0, nullptr, GL_FALSE);
        glDebugMessageCallback(SDL_GLDebugMessageCallback, nullptr);
    }

    glGenVertexArrays(1, &emptyVAO); 

    //glEnable(GL_CULL_FACE);
    //glCullFace(GL_BACK);
    //glEnable(GL_DEPTH_TEST);

    m_camera.SetView(glm::vec3(2, 2, 2),   // From where we look at the scene - eye
                     glm::vec3(0, 0, 0),   // Which point of the scene we are looking at - at
                     glm::vec3(0, 1, 0)    // Upwards direction - up
    );

    m_camera_manipulator.SetCamera(&m_camera);
}

App::~App() {
    glDeleteVertexArrays(1, &emptyVAO);
}

void App::Update(float elapsed_time_in_seconds, float delta_time_in_seconds) {
    m_time_in_seconds = elapsed_time_in_seconds;
    m_camera_manipulator.Update(delta_time_in_seconds);
}


void App::Render() {
    glDisable(GL_DEPTH_TEST);
    m_ray_tracer_shader.Use();
    glBindVertexArray(emptyVAO); 

    glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_skybox.GetTextureID());
    glUniform1i(m_ray_tracer_shader.ul("skyboxTexture"), 0);

    glUniformMatrix4fv(m_ray_tracer_shader.ul("inv_view_proj_mat"), 1, GL_FALSE, glm::value_ptr(glm::inverse(m_camera.GetViewProj())));
    glUniform3fv(m_ray_tracer_shader.ul("position"), 1, glm::value_ptr(m_camera.GetEye()));
    glUniform1f(m_ray_tracer_shader.ul("width"), static_cast<GLfloat>(m_width));
    glUniform1f(m_ray_tracer_shader.ul("height"), static_cast<GLfloat>(m_height));

    //glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glUseProgram(0);
    glBindVertexArray(0);
}

void App::RenderImGui() {
    ImGui::ShowDemoWindow();
    //if (ImGui::Begin("Settings")) {
    //}
    ImGui::End();
}

void App::KeyboardDown(const SDL_KeyboardEvent& key) {
    m_camera_manipulator.KeyboardDown(key);
}

void App::KeyboardUp(const SDL_KeyboardEvent& key) {
    m_camera_manipulator.KeyboardUp(key);
}

void App::MouseMove(const SDL_MouseMotionEvent& mouse) {
    m_camera_manipulator.MouseMove(mouse);
}

void App::MouseWheel(const SDL_MouseWheelEvent& wheel) {
    m_camera_manipulator.MouseWheel(wheel);
}

void App::Resize(GLsizei width, GLsizei height) {
    m_width = width;
    m_height = height;
    glViewport(0, 0, width, height);
    //m_framebuffer.Resize(width, height);
    m_camera.SetAspect(static_cast<float>(width) / static_cast<float>(width));
}