// HW2_TODO: Implement the RBFWarper class
#pragma once

#include "common/image_widget.h"  // Assuming ImVec2 is defined in imgui.h
#include "warper.h"
namespace USTC_CG
{
class RBFWarper : public Warper
{
   public:
    RBFWarper(
        const std::vector<ImVec2>& start_points,
        const std::vector<ImVec2>& end_points);
    virtual ~RBFWarper() = default;
    // HW2_TODO: Implement the warp(...) function with RBF interpolation
    std::pair<float, float> warp(float x, float y) override;

   private:
    std::vector<ImVec2> start_points_;
    std::vector<ImVec2> end_points_;
    // HW2_TODO: other functions or variables if you need
   private:
    std::vector<float> alpha_x_;              // RBF x方向权重
    std::vector<float> alpha_y_;              // RBF y方向权重
    float A_[2][2] = { { 1, 0 }, { 0, 1 } };  // 仿射矩阵
    ImVec2 b_{ 0, 0 };                        // 平移向量
};
}  // namespace USTC_CG