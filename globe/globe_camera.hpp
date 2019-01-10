//
// Project:                 LunarGlobe
// SPDX-License-Identifier: Apache-2.0
//
// File:                    globe/globe_camera.hpp
// Copyright(C):            2018-2019; LunarG, Inc.
// Author(s):               Mark Young <marky@lunarg.com>
//

#pragma once

#include <cstdint>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/euler_angles.hpp>

class GlobeCamera {
   public:
    GlobeCamera();
    virtual ~GlobeCamera() {}

    void SetPerspectiveProjection(float aspect_ratio, float field_of_view, float near, float far);
    void SetFrustumProjection(float left, float right, float top, float bottom, float near, float far);
    void SetOrthographicProjection(float left, float right, float top, float bottom, float near, float far);

    void SetCameraOrientation(float yaw, float pitch, float roll);
    void SetCameraPosition(float camera_x, float camera_y, float camera_z);

    glm::mat4 ViewMatrix();
    const glm::mat4* ProjectionMatrix() { return &_projection_matrix; }

   protected:
    glm::mat4 _projection_matrix;
    glm::vec3 _camera_position;
    glm::vec3 _camera_orientation;
};
