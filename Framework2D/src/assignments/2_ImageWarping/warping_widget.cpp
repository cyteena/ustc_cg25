#include "warping_widget.h"

#include <algorithm>
#include <cmath>
#include <iostream>

#include "warper/IDW_warper.h"
#include "warper/RBF_warper.h"

namespace USTC_CG
{
using uchar = unsigned char;

WarpingWidget::WarpingWidget(
    const std::string& label,
    const std::string& filename)
    : ImageWidget(label, filename)
{
    if (data_)
        back_up_ = std::make_shared<Image>(*data_);
    annoy_index_ = new Annoy::AnnoyIndex<
        int,
        double,
        Annoy::Euclidean,
        Annoy::Kiss32Random,
        Annoy::AnnoyIndexSingleThreadedBuildPolicy>(2);
}

void WarpingWidget::draw()
{
    // Draw the image
    ImageWidget::draw();
    // Draw the canvas
    if (flag_enable_selecting_points_)
        select_points();
}

void WarpingWidget::build_annoy_index()
{
    if (!data_)
        return;

    const int w = data_->width();
    const int h = data_->height();

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            double point[2] = { static_cast<double>(x),
                                static_cast<double>(y) };
            annoy_index_->add_item(y * w + x, point);
        }
    }
    annoy_index_->build(10, 1);  // 构建10棵树
    index_built_ = true;
}

void WarpingWidget::invert()
{
    for (int i = 0; i < data_->width(); ++i)
    {
        for (int j = 0; j < data_->height(); ++j)
        {
            const auto color = data_->get_pixel(i, j);
            data_->set_pixel(
                i,
                j,
                { static_cast<uchar>(255 - color[0]),
                  static_cast<uchar>(255 - color[1]),
                  static_cast<uchar>(255 - color[2]) });
        }
    }
    // After change the image, we should reload the image data to the renderer
    update();
}
void WarpingWidget::mirror(bool is_horizontal, bool is_vertical)
{
    Image image_tmp(*data_);
    int width = data_->width();
    int height = data_->height();

    if (is_horizontal)
    {
        if (is_vertical)
        {
            for (int i = 0; i < width; ++i)
            {
                for (int j = 0; j < height; ++j)
                {
                    data_->set_pixel(
                        i,
                        j,
                        image_tmp.get_pixel(width - 1 - i, height - 1 - j));
                }
            }
        }
        else
        {
            for (int i = 0; i < width; ++i)
            {
                for (int j = 0; j < height; ++j)
                {
                    data_->set_pixel(
                        i, j, image_tmp.get_pixel(width - 1 - i, j));
                }
            }
        }
    }
    else
    {
        if (is_vertical)
        {
            for (int i = 0; i < width; ++i)
            {
                for (int j = 0; j < height; ++j)
                {
                    data_->set_pixel(
                        i, j, image_tmp.get_pixel(i, height - 1 - j));
                }
            }
        }
    }

    // After change the image, we should reload the image data to the renderer
    update();
}
void WarpingWidget::gray_scale()
{
    for (int i = 0; i < data_->width(); ++i)
    {
        for (int j = 0; j < data_->height(); ++j)
        {
            const auto color = data_->get_pixel(i, j);
            uchar gray_value = (color[0] + color[1] + color[2]) / 3;
            data_->set_pixel(i, j, { gray_value, gray_value, gray_value });
        }
    }
    // After change the image, we should reload the image data to the renderer
    update();
}
void WarpingWidget::warping()
{
    // HW2_TODO: You should implement your own warping function that interpolate
    // the selected points.
    // Please design a class for such warping operations, utilizing the
    // encapsulation, inheritance, and polymorphism features of C++.

    // Create a new image to store the result
    Image warped_image(*data_);
    // Initialize the color of result image
    for (int y = 0; y < data_->height(); ++y)
    {
        for (int x = 0; x < data_->width(); ++x)
        {
            warped_image.set_pixel(x, y, { 0, 0, 0 });
        }
    }

    switch (warping_type_)
    {
        case kDefault: break;
        case kFisheye:
        {
            // Example: (simplified) "fish-eye" warping
            // For each (x, y) from the input image, the "fish-eye" warping
            // transfer it to (x', y') in the new image: Note: For this
            // transformation ("fish-eye" warping), one can also calculate the
            // inverse (x', y') -> (x, y) to fill in the "gaps".
            for (int y = 0; y < data_->height(); ++y)
            {
                for (int x = 0; x < data_->width(); ++x)
                {
                    // Apply warping function to (x, y), and we can get (x', y')
                    auto [new_x, new_y] =
                        fisheye_warping(x, y, data_->width(), data_->height());
                    // Copy the color from the original image to the result
                    // image
                    if (new_x >= 0 && new_x < data_->width() && new_y >= 0 &&
                        new_y < data_->height())
                    {
                        std::vector<unsigned char> pixel =
                            data_->get_pixel(x, y);
                        warped_image.set_pixel(new_x, new_y, pixel);
                    }
                }
            }
            break;
        }
        case kIDW:
        {
            // HW2_TODO: Implement the IDW warping
            // use selected points start_points_, end_points_ to construct the
            // map
            IDWWarper warper(end_points_, start_points_);
            for (int y = 0; y < data_->height(); y++)
            {
                for (int x = 0; x < data_->width(); x++)
                {
                    auto [src_x, src_y] = warper.warp(x, y);
                    auto pixel = nearest_neighbor_interpolation(src_x, src_y);
                    warped_image.set_pixel(x, y, pixel);
                }
            }
            break;
        }
        case kRBF:
        {
            // HW2_TODO: Implement the RBF warping
            // use selected points start_points_, end_points_ to construct the
            // map
            if (start_points_.size() < 1)
            {  // 添加控制点数量检查
                std::cout << "Need at least 1 control point for RBF warping"
                          << std::endl;
                return;
            }
            RBFWarper warper(end_points_, start_points_);
            for (int y = 0; y < data_->height(); y++)
            {
                for (int x = 0; x < data_->width(); x++)
                {
                    auto [src_x, src_y] = warper.warp(x, y);
                    auto pixel = nearest_neighbor_interpolation(src_x, src_y);
                    warped_image.set_pixel(x, y, pixel);
                }
            }
            break;
        }
        default: break;
    }

    *data_ = std::move(warped_image);
    update();
}
void WarpingWidget::restore()
{
    *data_ = *back_up_;
    update();
}
void WarpingWidget::set_default()
{
    warping_type_ = kDefault;
}
void WarpingWidget::set_fisheye()
{
    warping_type_ = kFisheye;
}
void WarpingWidget::set_IDW()
{
    warping_type_ = kIDW;
}
void WarpingWidget::set_RBF()
{
    warping_type_ = kRBF;
}
void WarpingWidget::enable_selecting(bool flag)
{
    flag_enable_selecting_points_ = flag;
}
void WarpingWidget::select_points()
{
    /// Invisible button over the canvas to capture mouse interactions.
    ImGui::SetCursorScreenPos(position_);
    ImGui::InvisibleButton(
        label_.c_str(),
        ImVec2(
            static_cast<float>(image_width_),
            static_cast<float>(image_height_)),
        ImGuiButtonFlags_MouseButtonLeft);
    // Record the current status of the invisible button
    bool is_hovered_ = ImGui::IsItemHovered();
    // Selections
    ImGuiIO& io = ImGui::GetIO();
    if (is_hovered_ && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        draw_status_ = true;
        start_ = end_ =
            ImVec2(io.MousePos.x - position_.x, io.MousePos.y - position_.y);
    }
    if (draw_status_)
    {
        end_ = ImVec2(io.MousePos.x - position_.x, io.MousePos.y - position_.y);
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            start_points_.push_back(start_);
            end_points_.push_back(end_);
            draw_status_ = false;
        }
    }
    // Visualization
    auto draw_list = ImGui::GetWindowDrawList();
    for (size_t i = 0; i < start_points_.size(); ++i)
    {
        ImVec2 s(
            start_points_[i].x + position_.x, start_points_[i].y + position_.y);
        ImVec2 e(
            end_points_[i].x + position_.x, end_points_[i].y + position_.y);
        draw_list->AddLine(s, e, IM_COL32(255, 0, 0, 255), 2.0f);
        draw_list->AddCircleFilled(s, 4.0f, IM_COL32(0, 0, 255, 255));
        draw_list->AddCircleFilled(e, 4.0f, IM_COL32(0, 255, 0, 255));
    }
    if (draw_status_)
    {
        ImVec2 s(start_.x + position_.x, start_.y + position_.y);
        ImVec2 e(end_.x + position_.x, end_.y + position_.y);
        draw_list->AddLine(s, e, IM_COL32(255, 0, 0, 255), 2.0f);
        draw_list->AddCircleFilled(s, 4.0f, IM_COL32(0, 0, 255, 255));
    }
}
void WarpingWidget::init_selections()
{
    start_points_.clear();
    end_points_.clear();
}

std::pair<int, int> WarpingWidget::fisheye_warping(
    int& x,
    int& y,
    const int& width,
    const int& height)
{
    float center_x = width / 2.0f;
    float center_y = height / 2.0f;
    float dx = x - center_x;
    float dy = y - center_y;
    float distance = std::sqrt(dx * dx + dy * dy);

    // Simple non-linear transformation r -> r' = f(r)
    float new_distance = std::sqrt(distance) * 10;

    if (distance == 0)
    {
        return { static_cast<int>(center_x), static_cast<int>(center_y) };
    }
    // (x', y')
    float ratio = new_distance / distance;
    int new_x = static_cast<int>(center_x + dx * ratio);
    int new_y = static_cast<int>(center_y + dy * ratio);

    return { new_x, new_y };
}

std::vector<uchar> WarpingWidget::bilinear_interpolation(float& x, float& y)
{
    int x0 = std::floor(x);
    int y0 = std::floor(y);
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    // 边界检查
    x0 = std::clamp(x0, 0, data_->width() - 1);
    x1 = std::clamp(x1, 0, data_->width() - 1);
    y0 = std::clamp(y0, 0, data_->height() - 1);
    y1 = std::clamp(y1, 0, data_->height() - 1);

    float dx = x - x0;
    float dy = y - y0;

    auto p00 = data_->get_pixel(x0, y0);
    auto p01 = data_->get_pixel(x0, y1);
    auto p10 = data_->get_pixel(x1, y0);
    auto p11 = data_->get_pixel(x1, y1);

    std::vector<uchar> result(3);
    for (int i = 0; i < 3; ++i)
    {
        float val = (1 - dx) * (1 - dy) * p00[i] + (1 - dx) * dy * p01[i] +
                    dx * (1 - dy) * p10[i] + dx * dy * p11[i];
        result[i] = static_cast<uchar>(std::clamp(val, 0.0f, 255.0f));
    }
    return result;
}

std::vector<uchar> WarpingWidget::nearest_neighbor_interpolation(
    float& x,
    float& y)
{
    int nearest_x = static_cast<int>(std::round(x));
    int nearest_y = static_cast<int>(std::round(y));

    nearest_x = std::clamp(nearest_x, 0, data_->width() - 1);
    nearest_y = std::clamp(nearest_y, 0, data_->height() - 1);

    return data_->get_pixel(nearest_x, nearest_y);
}

std::vector<uchar> WarpingWidget::ann_nearest_neighbor_interpolation(float& x, float& y)
{
    if (!index_built_) {
        build_annoy_index();
    }

    double query[2] = {static_cast<double>(x), static_cast<double>(y)};
    std::vector<int> result_ids;    // 改用vector接收结果
    std::vector<double> distances;
    
    // 查找最近邻
    annoy_index_->get_nns_by_vector(query, 1, -1, &result_ids, &distances);

    // 将线性索引转换为坐标
    const int w = data_->width();

    if (!result_ids.empty()){
        int nearest_id = result_ids[0];
        int nearest_x = static_cast<int>(std::round(nearest_id % w));
        int nearest_y = static_cast<int>(std::round(nearest_id / w));
    
        return data_->get_pixel(
            std::clamp(nearest_x, 0, w-1), 
            std::clamp(nearest_y, 0, data_->height()-1)
        );
    }
    else{
        return {0,0,0};
    }
}  // namespace USTC_CG
}