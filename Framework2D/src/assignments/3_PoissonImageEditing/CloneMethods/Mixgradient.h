#pragma once

#include "Seamless.h"
#include "clonemethod.h"
#include "common/image_widget.h"

namespace USTC_CG {

class MixGradient : public Seamless {
public:
    // 显式构造函数：注意参数类型为 std::shared_ptr<Image>
    MixGradient(const std::shared_ptr<Image>& source_image,
                const std::shared_ptr<Image>& target_image,
                const std::shared_ptr<Image>& mask,
                int offset_x,
                int offset_y)
        : Seamless(source_image, target_image, mask, offset_x, offset_y) {}

    std::shared_ptr<Image> solve() override;

private:
    void build_poisson_equation() override;
};

} // namespace USTC_CG