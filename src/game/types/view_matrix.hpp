#pragma once

#include <cmath>

#include "vector2d.hpp"
#include "vector3d.hpp"

struct view_matrix_t {
    float matrix[4][4]{};

    const float* operator[](int index) const { return matrix[index]; }
    float* operator[](int index) { return matrix[index]; }

    // World-to-screen. Returns false when the point is behind the camera, is
    // outside of the viewport, or produced a non-finite coordinate.
    bool wts(const Vec3_t& pos, const Vec2_t& screen, Vec2_t& out, bool check_bounds = true) const
    {
        const float w = matrix[3][0] * pos.x + matrix[3][1] * pos.y + matrix[3][2] * pos.z + matrix[3][3];

        if (w <= 0.01f)
            return false;

        const float inv_w = 1.0f / w;
        const float half_width = screen.x * 0.5f;
        const float half_height = screen.y * 0.5f;

        out.x = half_width + (matrix[0][0] * pos.x + matrix[0][1] * pos.y + matrix[0][2] * pos.z + matrix[0][3]) * inv_w * half_width;
        out.y = half_height - (matrix[1][0] * pos.x + matrix[1][1] * pos.y + matrix[1][2] * pos.z + matrix[1][3]) * inv_w * half_height;

        if (!std::isfinite(out.x) || !std::isfinite(out.y))
            return false;

        if (check_bounds &&
            (
                out.x < 0.f || out.x > screen.x ||
                out.y < 0.f || out.y > screen.y
            ))
            return false;

        return true;
    }
};
