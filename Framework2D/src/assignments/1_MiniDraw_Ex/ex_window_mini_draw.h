#pragma once

#include <memory>

#include "../1_MiniDraw/canvas_widget.h"
#include "common/window.h"

namespace USTC_CG
{
class MiniDraw : public Window
{
   public:
    explicit MiniDraw(const std::string& window_name);
    ~MiniDraw();

    void draw() override;

   private:
    void draw_canvas();

    std::shared_ptr<Canvas> p_canvas_ = nullptr;

    bool flag_show_canvas_view_ = true;
};
void MiniDraw::draw()
{
    // flag_show_canvas_common : bool type
    if (ImGui::Begin("Canvas", &flag_show_canvas_view_))
    {
        ImGui::Text("This is a canvas window");
    }
    ImGui::End();
}
}  // namespace USTC_CG
