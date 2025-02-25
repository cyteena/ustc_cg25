#include "ellipse.h"

#include <imgui.h>
#include <math.h>
namespace USTC_CG
{

// Draw the ellipse using ImGui
void Ellipse::draw(const Config& config) const
{
    ImDrawList* drawlist = ImGui::GetWindowDrawList();

    drawlist->AddEllipse(
        ImVec2(config.bias[0] + start_point_x_, config.bias[1] + start_point_y_),
        ImVec2(abs(radius_x_), abs(radius_y_)),  //
        IM_COL32(
            config.line_color[0],
            config.line_color[1],
            config.line_color[2],
            config.line_color[3]),
            rotation_angle_,
            num_segments,
        config.line_thickness

    );
}
void Ellipse::update(float x, float y)
{ // x, y : end_point_x_, end_point_y_
    const bool is_rotating = ImGui::GetIO().KeyAlt;

    if (is_rotating)
    {
        float dx = x - start_point_x_;
        float dy = y - start_point_y_;
        rotation_angle_ = atan2f(dy, dx);
        radius_x_  = dx;
        radius_y_  = dy;
    }
    else
    {
        radius_x_ = x - start_point_x_;
        radius_y_ = y - start_point_y_;
    }

}  // namespace USTC_CG
}  // namespace USTC_CG