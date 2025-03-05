// HW2_TODO: Implement the NNWarper class
#pragma once

#include <dlib/dnn.h>

#include "common/image_widget.h"  // Assuming ImVec2 is defined in imgui.h
#include "warper.h"

namespace USTC_CG
{
class NNWarper : public Warper
{
   public:
    NNWarper(
        const std::vector<ImVec2>& start_points,
        const std::vector<ImVec2>& end_points);
    virtual ~NNWarper() = default;
    // HW2_TODO: Implement the warp(...) function with IDW interpolation
    std::pair<float, float> warp(float x, float y) override;

   private:
    std::vector<ImVec2> start_points_;
    std::vector<ImVec2> end_points_;

    float x_scale_ = 1.0f;
    float y_scale_ = 1.0f;
    float x_offset_ = 0.0f;
    float y_offset_ = 0.0f;
    using net_type = dlib::loss_mean_squared_multioutput<dlib::fc<
        2,
        dlib::elu<dlib::fc<
            10,
            dlib::elu<dlib::fc<10, dlib::input<dlib::matrix<float>>>>>>>>;

    net_type net_;

    // HW2_TODO: other functions or variables if you need
};
}  // namespace USTC_CG