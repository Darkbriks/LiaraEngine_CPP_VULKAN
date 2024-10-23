//
// Created by antoi on 21/10/2024.
//

#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace Liara
{
    class Liara_Camera
    {
    public:
        void SetOrthographicProjection(float left, float right, float top, float bottom, float near, float far);
        void SetPerspectiveProjection(float fovy, float aspect, float near, float far);

        [[nodiscard]] glm::mat4 GetProjectionMatrix() const { return m_ProjectionMatrix; }
        [[nodiscard]] glm::mat4 GetViewMatrix() const { return m_ViewMatrix; }

        void SetViewDirection(const glm::vec3& position, const glm::vec3& direction, const glm::vec3& up = {0.0f, -1.0f, 0.0f});
        void SetViewTarget(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up = {0.0f, -1.0f, 0.0f});
        void SetViewYXZ(const glm::vec3& position, const glm::vec3& rotation);

    private:
        glm::mat4 m_ProjectionMatrix{1.0f};
        glm::mat4 m_ViewMatrix{1.0f};
    };
} // Liara
