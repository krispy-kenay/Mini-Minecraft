#include "camera.h"
#include "glm_includes.h"

Camera::Camera(glm::vec3 pos)
    : Camera(400, 400, pos)
{}

Camera::Camera(unsigned int w, unsigned int h, glm::vec3 pos)
    : Entity(pos), m_fovy(45), m_width(w), m_height(h),
      m_near_clip(0.1f), m_far_clip(1000.f), m_aspect(w / static_cast<float>(h))
{}

Camera::Camera(const Camera &c)
    : Entity(c),
      m_fovy(c.m_fovy),
      m_width(c.m_width),
      m_height(c.m_height),
      m_near_clip(c.m_near_clip),
      m_far_clip(c.m_far_clip),
      m_aspect(c.m_aspect)
{}


void Camera::setWidthHeight(unsigned int w, unsigned int h) {
    m_width = w;
    m_height = h;
    m_aspect = w / static_cast<float>(h);
}


void Camera::tick(float dT, InputBundle &input) {
    // Do nothing
}

glm::mat4 Camera::getViewProj() const {
    return glm::perspective(glm::radians(m_fovy), m_aspect, m_near_clip, m_far_clip) * glm::lookAt(m_position, m_position + m_forward, m_up);
}

std::array<glm::vec4, 6> Camera::getFrustumPlanes() const {
    glm::mat4 vp = getViewProj();
    std::array<glm::vec4, 6> planes;
    
    // Left plane
    planes[0] = glm::vec4(vp[0][3] + vp[0][0], 
                         vp[1][3] + vp[1][0], 
                         vp[2][3] + vp[2][0], 
                         vp[3][3] + vp[3][0]);
    
    // Right plane
    planes[1] = glm::vec4(vp[0][3] - vp[0][0], 
                         vp[1][3] - vp[1][0], 
                         vp[2][3] - vp[2][0], 
                         vp[3][3] - vp[3][0]);
    
    // Bottom plane
    planes[2] = glm::vec4(vp[0][3] + vp[0][1], 
                         vp[1][3] + vp[1][1], 
                         vp[2][3] + vp[2][1], 
                         vp[3][3] + vp[3][1]);
    
    // Top plane
    planes[3] = glm::vec4(vp[0][3] - vp[0][1], 
                         vp[1][3] - vp[1][1], 
                         vp[2][3] - vp[2][1], 
                         vp[3][3] - vp[3][1]);
    
    // Near plane
    planes[4] = glm::vec4(vp[0][3] + vp[0][2], 
                         vp[1][3] + vp[1][2], 
                         vp[2][3] + vp[2][2], 
                         vp[3][3] + vp[3][2]);
    
    // Far plane
    planes[5] = glm::vec4(vp[0][3] - vp[0][2], 
                         vp[1][3] - vp[1][2], 
                         vp[2][3] - vp[2][2], 
                         vp[3][3] - vp[3][2]);

    // Normalize the planes
    for (auto& plane : planes) {
        plane /= glm::length(glm::vec3(plane));
    }
    
    return planes;
}