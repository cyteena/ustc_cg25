#include <stdexcept>
#include <imgui.h>

#include "common/window.h"
#include "ex_window_mini_draw.h"


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