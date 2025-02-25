#pragma once

#include "shape.h"

namespace USTC_CG
{
class Ellipse : public Shape
{
   public:
    Ellipse() = default;

    // Constructor to initialize a epplipse with start and end
    Ellipse(float st_x, float st_y) : start_point_x_(st_x), start_point_y_(st_y)
    {
    }
    virtual ~Ellipse() = default;

    void draw(const Config& config) const override;

    void update(float x, float y) override;

   private:
    float start_point_x_, start_point_y_, end_point_x_, end_point_y_,
        radius_x_ = 1.0f, radius_y_ = 1.0f, rotation_angle_ = 0.0f, thick_ness = 2.0f;
    int num_segments = 0;
};
}  // namespace USTC_CG


// void ImDrawList::AddEllipse(const ImVec2& center, const ImVec2& radius, ImU32 col, float rot, int num_segments, float thickness)
// {
//     if ((col & IM_COL32_A_MASK) == 0)
//         return;

//     if (num_segments <= 0)
//         num_segments = _CalcCircleAutoSegmentCount(ImMax(radius.x, radius.y)); // A bit pessimistic, maybe there's a better computation to do here.

//     // Because we are filling a closed shape we remove 1 from the count of segments/points
//     const float a_max = IM_PI * 2.0f * ((float)num_segments - 1.0f) / (float)num_segments;
//     PathEllipticalArcTo(center, radius, rot, 0.0f, a_max, num_segments - 1);
//     PathStroke(col, true, thickness);
// }