// HW2_TODO: Implement the IDWWarper class
#pragma once

#include "common/image_widget.h"  // Assuming ImVec2 is defined in imgui.h
#include "warper.h"
namespace USTC_CG
{
class IDWWarper : public Warper
{
   public:
    IDWWarper(
        const std::vector<ImVec2>& start_points,
        const std::vector<ImVec2>& end_points);
    virtual ~IDWWarper() = default;
    // HW2_TODO: Implement the warp(...) function with IDW interpolation
    std::pair<float, float> warp(float x, float y) override;

   private:
    std::vector<ImVec2> start_points_;
    std::vector<ImVec2> end_points_;

    // HW2_TODO: other functions or variables if you need
};
}  // namespace USTC_CG