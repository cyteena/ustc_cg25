#pragma once
#include "common/image_widget.h"

namespace USTC_CG
{
class CloneMethod
{
   public:
    CloneMethod(
        std::shared_ptr<Image> src,
        std::shared_ptr<Image> dst,
        std::shared_ptr<Image> mask,
        int offset_x,
        int offset_y)
        : src_img_(src),
          tar_img_(dst),
          src_selected_mask(mask),
          offset_x_(offset_x),
          offset_y_(offset_y)
    {
    }
    virtual ~CloneMethod() = default;
    virtual std::shared_ptr<Image> solve() = 0;

   public:
    std::shared_ptr<Image> get_source_image() const
    {
        return src_img_;
    }
    std::shared_ptr<Image> get_target_image() const
    {
        return tar_img_;
    }
    std::shared_ptr<Image> get_mask() const
    {
        return src_selected_mask;
    }
    int get_offset_x() const
    {
        return offset_x_;
    }
    int get_offset_y() const
    {
        return offset_y_;
    }

   private:
    std::shared_ptr<Image> src_img_;
    std::shared_ptr<Image> tar_img_;
    std::shared_ptr<Image> src_selected_mask;

    int offset_x_, offset_y_;

};

}  // namespace USTC_CG