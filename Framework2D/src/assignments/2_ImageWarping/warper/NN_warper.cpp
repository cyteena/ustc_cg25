#include "NN_warper.h"

#include <cmath>
#include <iostream>

namespace USTC_CG
{
NNWarper::NNWarper(
    const std::vector<ImVec2>& start_points,
    const std::vector<ImVec2>& end_points)
    : start_points_(start_points),
      end_points_(end_points) { };
std::pair<float, float> NNWarper::warp(float x, float y)
{

}
}  // namespace USTC_CG