#pragma once

#include <vector>

#include "shape.h"

namespace USTC_CG
{

class Freehand : public Shape
{
   private:
    std::vector<float> x_list_, y_list_;

   public:
    Freehand() = default;
    Freehand(std::vector<float> x_list_, std::vector<float> y_list_);
    ~Freehand() override = default;

    void draw(const Config& config) const override;

    void update(float x, float y) override;

    std::vector<std::pair<int, int>> get_interior_pixels() const override;
};
}  // namespace USTC_CG
