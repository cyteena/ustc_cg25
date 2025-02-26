#pragma once

#include <vector>

#include "shape.h"

namespace USTC_CG
{
class Polygon : public Shape
{
   public:
    Polygon() = default;

    // Initialize a Polygonangle with start and end points
    Polygon(std::vector<float> x_list, std::vector<float> y_list) { };

    ~Polygon() override = default;

    // Draws the Polygonangle on the screen
    // Overrides draw function to implement Polygonangle-specific drawing logic
    void draw(const Config& config) const override;

    // Overrides Shape's update function to adjust the Polygonangle size during
    // interaction
    void update(float x, float y) override;
    void add_control_point(float x, float y) override;

   private:
    // Coordinates of the top-left and bottom-right corners of the Polygonangle
    std::vector<float> x_list_, y_list_;

   public:
    ControlPoint get_control_point(int index) const override;
    int get_control_points_count() const override;
};
}  // namespace USTC_CG