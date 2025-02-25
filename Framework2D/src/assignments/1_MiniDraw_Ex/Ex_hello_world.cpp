#include <imgui.h>

#include <stdexcept>

#include "common/window.h"
#include "ex_window_mini_draw.h"

void USTC_CG::MiniDraw::draw()
{
    // flag_show_canvas_common : bool type
    if (ImGui::Begin("Canvas", &flag_show_canvas_view_))
    {
        ImGui::Text("This is a canvas window");
    }
    ImGui::End();
}

int main()
{
    try
    {
        USTC_CG::Window w("Ex1_Hello_world");
        if (!w.init())
        {
            return 1;
        }
        w.run();
        return 0;
    }
    catch (const std::exception& e)
    {
        fprintf(stderr, "Error: %s\n", e.what());
        return 1;
    }
}