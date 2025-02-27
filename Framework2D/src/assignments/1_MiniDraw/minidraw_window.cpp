#include "minidraw_window.h"

#include <iostream>

namespace USTC_CG
{
MiniDraw::MiniDraw(const std::string& window_name)
    : Window(window_name)  // 调用基类构造函数，传递窗口标题
{
    // 创建画布组件实例
    // 使用智能指针管理画布生命周期，确保窗口销毁时自动释放资源
    p_canvas_ = std::make_shared<Canvas>("Widget.Canvas");
}

MiniDraw::~MiniDraw()
{
}

void MiniDraw::draw()
{
    draw_canvas();
}

void MiniDraw::draw_canvas()
{
    // 设置全屏画布视图
    // 获取主视口信息，用于后续窗口定位
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    // 将画布窗口定位到视口工作区原点
    ImGui::SetNextWindowPos(viewport->WorkPos);
    // 设置画布窗口尺寸与视口工作区一致
    ImGui::SetNextWindowSize(viewport->WorkSize);

    // 创建无装饰背景的画布窗口
    if (ImGui::Begin(
            "Canvas",
            &flag_show_canvas_view_,  // 窗口可见性控制标志
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground))
    {
        // 形状工具选择区
        // 线条工具按钮
        if (ImGui::Button("Line"))
        {
            std::cout << "Set shape to Line" << std::endl;
            p_canvas_->set_line();  // 切换画布绘制模式为线段
        }
        ImGui::SameLine();
        // 矩形工具按钮
        if (ImGui::Button("Rect"))
        {
            std::cout << "Set shape to Rect" << std::endl;
            p_canvas_->set_rect();  // 切换画布绘制模式为矩形
        }
        ImGui::SameLine();
        if (ImGui::Button("Ellipse"))
        {
            std::cout << "Set shape to Ellipse" << std::endl;
            p_canvas_->set_ellipse();
        }
        ImGui::SameLine();
        if (ImGui::Button("Polygon"))
        {
            std::cout << "Set shape to Polygon" << std::endl;
            p_canvas_->set_polygon();
        }
        ImGui::SameLine();
        if (ImGui::Button("Freehand"))
        {
            std::cout << "Free to draw" << std::endl;
            p_canvas_->set_freehand();
        }
        ImGui::SameLine();  // 保持按钮水平排列

        // 作业扩展接口：需添加椭圆/多边形等基本图形支持
        // HW1_TODO: More primitives
        //    - Ellipse
        //    - Polygon
        //    - Freehand(optional)

        // 画布操作提示
        ImGui::Text("Press left mouse to add shapes.");
        // 添加多边形专用提示
        if (p_canvas_->get_shape_type() == USTC_CG::Canvas::kPolygon)
        {
            ImGui::SameLine();
            ImGui::TextColored(
                ImVec4(1, 0, 0, 1), "[Right Click] to finish polygon");
        }

        // 画布区域设置
        // 获取可用区域起始坐标（排除工具栏后的剩余空间）
        const auto& canvas_min = ImGui::GetCursorScreenPos();
        // 获取可用区域尺寸
        const auto& canvas_size = ImGui::GetContentRegionAvail();
        // 将坐标尺寸传递给画布组件
        p_canvas_->set_attributes(canvas_min, canvas_size);
        // 执行画布绘制操作
        p_canvas_->draw();
    }
    ImGui::End();  // 结束画布窗口定义
}
}  // namespace USTC_CG