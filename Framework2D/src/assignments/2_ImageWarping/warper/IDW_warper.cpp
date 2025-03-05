#include "IDW_warper.h"

#include <cmath>
#include <iostream>

namespace USTC_CG
{
IDWWarper::IDWWarper(
    const std::vector<ImVec2>& start_points,
    const std::vector<ImVec2>& end_points)
    : start_points_(start_points),
      end_points_(end_points) { };
std::pair<float, float> IDWWarper::warp(float x, float y)
{
    float sum_w = 0.0f;
    float sum_wx = 0.0f;
    float sum_wy = 0.0f;
    constexpr float mu = 2.0f;
    constexpr float epsilon = 1e-9f;

    for (size_t i = 0; i < start_points_.size(); i++)
    {
        // calculate the drift to the control point
        float dx = x - static_cast<float>(start_points_[i].x);
        float dy = y - static_cast<float>(start_points_[i].y);

        float dist_sq = dx * dx + dy * dy;

        // calculate the weight term sigma_i = 1/(dist^u + epsilon)
        float sigma = 1.0f / (powf(dist_sq, mu / 2) + epsilon);

        // f(p) = \sum_{i=1}^n w_i(p)q_i
        // w_i(p) = sigma_i(p) / sum_sigma
        sum_w += sigma;
        sum_wx += sigma * static_cast<float>(end_points_[i].x - start_points_[i].x);
        sum_wy += sigma * static_cast<float>(end_points_[i].y - start_points_[i].y);

    }
    if (sum_w < epsilon)
    {
        return { x, y };
    }
    return { x + sum_wx / sum_w, y + sum_wy / sum_w };
}
}  // namespace USTC_CG