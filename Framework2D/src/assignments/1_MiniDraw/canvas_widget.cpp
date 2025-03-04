#include "canvas_widget.h"

#include <cmath>
#include <iostream>

#include "imgui.h"
#include "shapes/ellipse.h"
#include "shapes/freehand.h"
#include "shapes/line.h"
#include "shapes/polygon.h"
#include "shapes/rect.h"

namespace USTC_CG
{
void Canvas::draw()
{
    draw_background();
    // HW1_TODO: more interaction events
    if (is_hovered_ && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        mouse_click_event();
    if (is_hovered_ && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        mouse_click_right_event();
    mouse_move_event();
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
        mouse_release_event();

    draw_shapes();
}

void Canvas::set_attributes(const ImVec2& min, const ImVec2& size)
{
    canvas_min_ = min;
    canvas_size_ = size;
    canvas_minimal_size_ = size;
    canvas_max_ =
        ImVec2(canvas_min_.x + canvas_size_.x, canvas_min_.y + canvas_size_.y);
}

void Canvas::show_background(bool flag)
{
    show_background_ = flag;
}

void Canvas::set_default()
{
    draw_status_ = false;
    shape_type_ = kDefault;
}

void Canvas::set_line()
{
    draw_status_ = false;
    shape_type_ = kLine;
}

void Canvas::set_rect()
{
    draw_status_ = false;
    shape_type_ = kRect;
}

// HW1_TODO: more shape types, implements
void Canvas::set_ellipse()
{
    draw_status_ = false;
    shape_type_ = kEllipse;
}

void Canvas::set_polygon()
{
    draw_status_ = false;
    shape_type_ = kPolygon;
}

void Canvas::set_freehand()
{
    draw_status_ = false;
    shape_type_ = kFreehand;
}

void Canvas::clear_shape_list()
{
    shape_list_.clear();
}

void Canvas::draw_background()
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    if (show_background_)
    {
        // Draw background recrangle
        draw_list->AddRectFilled(canvas_min_, canvas_max_, background_color_);
        // Draw background border
        draw_list->AddRect(canvas_min_, canvas_max_, border_color_);
    }
    /// Invisible button over the canvas to capture mouse interactions.
    ImGui::SetCursorScreenPos(canvas_min_);
    ImGui::InvisibleButton(
        label_.c_str(), canvas_size_, ImGuiButtonFlags_MouseButtonLeft);
    // Record the current status of the invisible button
    is_hovered_ = ImGui::IsItemHovered();
    is_active_ = ImGui::IsItemActive();
}

void Canvas::draw_shapes()
{
    Shape::Config s = { .bias = { canvas_min_.x, canvas_min_.y } };
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // ClipRect can hide the drawing content outside of the rectangular area
    draw_list->PushClipRect(canvas_min_, canvas_max_, true);
    for (const auto& shape : shape_list_)
    {
        shape->draw(s);
    }
    if (draw_status_ && current_shape_)
    {
        current_shape_->draw(s);
    }
    draw_list->PopClipRect();
}

void Canvas::mouse_click_event()
{
    // HW1_TODO: Drawing rule for more primitives
    if (!draw_status_ || shape_type_ == kPolygon)
    {
        draw_status_ = true;
        start_point_ = end_point_ = mouse_pos_in_canvas();
        switch (shape_type_)
        {
            case USTC_CG::Canvas::kDefault:
            {
                break;
            }
            case USTC_CG::Canvas::kLine:
            {
                current_shape_ = std::make_shared<Line>(
                    start_point_.x, start_point_.y, end_point_.x, end_point_.y);
                break;
            }
            case USTC_CG::Canvas::kRect:
            {
                current_shape_ = std::make_shared<Rect>(
                    start_point_.x, start_point_.y, end_point_.x, end_point_.y);
                break;
            }
            // HW1_TODO: case USTC_CG::Canvas::kEllipse:
            case USTC_CG::Canvas::kEllipse:
            {
                current_shape_ =
                    std::make_shared<Ellipse>(start_point_.x, start_point_.y);
                break;
            }
            case USTC_CG::Canvas::kPolygon:
            {
                if (!current_shape_)
                {
                    current_shape_ = std::make_shared<Polygon>();
                }
                current_shape_->add_control_point(
                    start_point_.x, start_point_.y);
                break;
            }
            case USTC_CG::Canvas::kFreehand:
            {
                if (!current_shape_)
                {
                    current_shape_ = std::make_shared<Freehand>();
                }
                current_shape_->update(
                    start_point_.x, start_point_.y
                );
                break;
            }
            default: break;
        }
    }
    else
    {
        if (shape_type_ != kPolygon)
        {
            draw_status_ = false;
            if (current_shape_)
            {
                shape_list_.push_back(current_shape_);
                current_shape_.reset();
            }
        }
    }
}

void Canvas::mouse_click_right_event()
{
    // 合并所有右键处理逻辑
    if (shape_type_ == kPolygon && current_shape_)
    {
        // 至少需要3个顶点才能闭合多边形
        if (current_shape_->get_control_points_count() >= 3)
        {
            // 添加首顶点闭合路径
            ControlPoint first_point = current_shape_->get_control_point(0);
            current_shape_->add_control_point(first_point.x, first_point.y);

            // 完成绘制并保存
            draw_status_ = false;
            shape_list_.push_back(current_shape_);
            current_shape_.reset();
        }
        else
        {
            std::cout << "至少需要3个顶点才能闭合多边形" << std::endl;
        }
    }
    else
    {
        // 其他图形的右键处理（如果有需要）
        std::cout << "当前模式不支持右键操作" << std::endl;
    }
}

void Canvas::mouse_move_event()
{
    // HW1_TODO: Drawing rule for more primitives
    if (draw_status_)
    {
        end_point_ = mouse_pos_in_canvas();
        if (current_shape_)
        {
            current_shape_->update(end_point_.x, end_point_.y);
        }
    }
}

void Canvas::mouse_release_event()
{
    // HW1_TODO: Drawing rule for more primitives
    // 这个函数只完成了多边形下的右键填满操作
    if (draw_status_ && (shape_type_ != kPolygon))
    {
        return;
    }

    // 多边形模式下右键点击完成绘制
    if (shape_type_ == kPolygon &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        if (current_shape_ && current_shape_->get_control_points_count() >= 3)
        {
            ControlPoint first_point = current_shape_->get_control_point(0);
            current_shape_->add_control_point(first_point.x, first_point.y);

            draw_status_ = false;
            shape_list_.push_back(current_shape_);
            current_shape_.reset();
        }
    }

}

ImVec2 Canvas::mouse_pos_in_canvas() const
{
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 mouse_pos_in_canvas(
        io.MousePos.x - canvas_min_.x, io.MousePos.y - canvas_min_.y);
    return mouse_pos_in_canvas;
}
}  // namespace USTC_CG