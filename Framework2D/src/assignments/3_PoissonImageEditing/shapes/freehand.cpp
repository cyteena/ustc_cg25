#include "freehand.h"

#include <imgui.h>
#include <algorithm>

namespace USTC_CG
{
void Freehand::draw(const Config& config) const
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    for (int i = 0; i < x_list_.size() - 1; i++)
    {
        draw_list->AddLine(
            ImVec2(config.bias[0] + x_list_[i], config.bias[1] + y_list_[i]),
            ImVec2(
                config.bias[0] + x_list_[i + 1],
                config.bias[1] + y_list_[i + 1]),
            IM_COL32(
                config.line_color[0],
                config.line_color[1],
                config.line_color[2],
                config.line_color[3]),
            config.line_thickness);
    }
}

void Freehand::update(float x, float y)
{
    x_list_.push_back(x);
    y_list_.push_back(y);
}

std::vector<std::pair<int, int>> Freehand::get_interior_pixels() const
{
    std::vector<std::pair<int, int>> interior_pixels;
    if (x_list_.size() < 3) return interior_pixels;

    // Find bounding box
    int min_x = *std::min_element(x_list_.begin(), x_list_.end());
    int max_x = *std::max_element(x_list_.begin(), x_list_.end());
    int min_y = *std::min_element(y_list_.begin(), y_list_.end());
    int max_y = *std::max_element(y_list_.begin(), y_list_.end());

    // Scan line by line
    for (int y = min_y; y <= max_y; ++y) {
        std::vector<int> intersections;
        
        // Find intersections with each edge
        for (size_t i = 0; i < y_list_.size(); i++) {
            size_t j = (i + 1) % y_list_.size();
            
            if ((y_list_[i] <= y && y_list_[j] > y) || 
                (y_list_[j] <= y && y_list_[i] > y)) {
                
                float x;
                if (y_list_[j] != y_list_[i]) {
                    x = x_list_[i] + (y - y_list_[i]) *
                                     (x_list_[j] - x_list_[i]) /
                                     (y_list_[j] - y_list_[i]);
                } else {
                    continue; // Skip this edge if it's horizontal
                }
                
                intersections.push_back(static_cast<int>(x));
            }
        }
        
        // Sort intersections
        std::sort(intersections.begin(), intersections.end());
        
        // Fill pixels between pairs of intersections
        for (size_t i = 0; i < intersections.size(); i += 2) {
            if (i + 1 >= intersections.size()) break;
            for (int x = intersections[i]; x <= intersections[i + 1]; ++x) {
                interior_pixels.emplace_back(x, y);
            }
        }
    }

    return interior_pixels;
}

}  // namespace USTC_CG
