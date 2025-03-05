#pragma once

#include "common/image_widget.h"
#include <annoylib.h>
#include <kissrandom.h>


namespace USTC_CG
{
// Image component for warping and other functions

using uchar = unsigned char;
class WarpingWidget : public ImageWidget
{
   public:
    explicit WarpingWidget(
        const std::string& label,
        const std::string& filename);
    virtual ~WarpingWidget() noexcept = default;

    void draw() override;

    // Simple edit functions
    void invert();
    void mirror(bool is_horizontal, bool is_vertical);
    void gray_scale();
    void warping();
    void restore();

    // Enumeration for supported warping types.
    // HW2_TODO: more warping types.
    enum WarpingType
    {
        kDefault = 0,
        kFisheye = 1,
        kIDW = 2,
        kRBF = 3,
        kNN = 4,
    };
    // Warping type setters.
    void set_default();
    void set_fisheye();
    void set_IDW();
    void set_RBF();
    void set_NN();

    // Point selecting interaction
    void enable_selecting(bool flag);
    void select_points();
    void init_selections();

   private:
    // Store the original image data
    std::shared_ptr<Image> back_up_;
    // The selected point couples for image warping
    std::vector<ImVec2> start_points_, end_points_;

    ImVec2 start_, end_;
    bool flag_enable_selecting_points_ = false;
    bool draw_status_ = false;
    WarpingType warping_type_;

    Annoy::AnnoyIndex<int, double, Annoy::Euclidean, Annoy::Kiss32Random, Annoy::AnnoyIndexSingleThreadedBuildPolicy>* annoy_index_;
    bool index_built_ = false;

   private:
    // A simple "fish-eye" warping function
    std::pair<int, int> fisheye_warping(int& x, int& y, const int& width, const int& height);
    std::vector<unsigned char> bilinear_interpolation(float& x, float& y);
    std::vector<uchar> nearest_neighbor_interpolation(float& x, float& y);
    std::vector<uchar> ann_nearest_neighbor_interpolation(float& x, float& y);
    void build_annoy_index();
};

}  // namespace USTC_CG