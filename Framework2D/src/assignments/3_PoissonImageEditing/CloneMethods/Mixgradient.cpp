#include "Mixgradient.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

#include "Log.h"

namespace USTC_CG
{

extern Logger logger;

void MixGradient::build_poisson_equation()
{
    // TODO: 实现 Mix Gradient 的泊松方程构建
    // 在这里实现你的 Mix Gradient 特定的泊松方程构建逻辑
    const auto& src = get_source_image();
    const auto& mask = get_mask();
    const auto& tar = get_target_image();
    const int width = mask->width();
    const int height = mask->height();

    logger.debug() << "Source image size: " << src->width() << "x"
                   << src->height() << std::endl;
    logger.debug() << "Mask size: " << width << "x" << height << std::endl;
    logger.debug() << "Building Poisson equation for seamless cloning..."
                   << std::endl;

    // step 1: construct the mapping for index
    // index_map only contain the index of mask pixel

    index_map.clear();
    int index = 0;
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            if (mask->get_pixel(x, y)[0] > 128)
            {
                index_map[y * width + x] = index++;
            }
        }
    }
    const int N = index_map.size();

    logger.debug() << "Created index mapping with " << N << " mask pixels"
                   << std::endl;

    A_.resize(N, N);
    b_ = Eigen::MatrixXd::Zero(N, 3);
    std::vector<Eigen::Triplet<double>> triplets;

    // traverse every pixel in the mask
    for (const auto& [pos, i] : index_map)
    {
        // the coordinate in source image + offset_value = the coordinate in
        // target image
        const int x = pos % width;
        const int y = pos / width;
        int neighbor_count = 0;

        // 处理四个邻居方向
        const std::vector<std::pair<int, int>> neighbors = {
            { x - 1, y }, { x + 1, y }, { x, y - 1 }, { x, y + 1 }
            // 左、右、上、下
        };

        for (const auto& [nx, ny] : neighbors)
        {
            if (nx >= 0 && nx < width && ny >= 0 && ny < height)
            {
                const int n_pos = ny * width + nx;

                // neighbor in the mask
                if (index_map.count(n_pos))
                {
                    // cache the index of current pixel and its neighbor
                    // pixel emplace_back: more efficient than push_back
                    triplets.emplace_back(
                        i,
                        index_map.at(n_pos),
                        -1.0);  // i: row index in the sparse matrix, second
                                // term represents the col index
                }
                // neighbor not in the mask
                else
                {
                    for (int c = 0; c < 3; ++c)
                    {
                        b_(i, c) += tar->get_pixel(
                            nx + get_offset_x(), ny + get_offset_y())[c];
                    }
                }
                neighbor_count++;
            }
        }
        // set the center_coeff
        triplets.emplace_back(i, i, neighbor_count);

        // consider the gradient of src image
        for (int c = 0; c < 3; ++c)
        {
            const double src_val = src->get_pixel(x, y)[c];
            const double tar_val =
                tar->get_pixel(x + get_offset_x(), y + get_offset_y())[c];
            for (const auto& [nx, ny] : neighbors)
            {
                if (nx >= 0 && nx < width && ny >= 0 && ny < height)
                {
                    if (std::abs(
                            tar_val -
                            tar->get_pixel(
                                nx + get_offset_x(), ny + get_offset_y())[c]) >
                        std::abs(src_val - src->get_pixel(nx, ny)[c]))
                    {
                        b_(i, c) += tar_val - tar->get_pixel(
                                                  nx + get_offset_x(),
                                                  ny + get_offset_y())[c];
                    }
                    else
                    {
                        b_(i, c) += src_val - src->get_pixel(nx, ny)[c];
                    }
                }
            }
        }

        if (neighbor_count < 4)
        {
            logger.trace() << "Pixel (" << x << "," << y << ") has "
                           << neighbor_count << " valid neighbors";
        }
    }
    A_.setFromTriplets(triplets.begin(), triplets.end());

    // 在边界条件处理处增加详细日志
    logger.info() << "Poisson equation built successfully. Non-zero elements: "
                  << A_.nonZeros() << ", Matrix size: " << A_.rows() << "x"
                  << A_.cols() << std::endl;
    std::cout << "MixGradient::build_poisson_equation() called" << std::endl;
}

std::shared_ptr<Image> MixGradient::solve()
{
    // TODO: 实现 Mix Gradient 的 solve 算法
    // 在这里实现你的 Mix Gradient 特定的求解逻辑
    std::cout << "MixGradient::solve() called" << std::endl;
    logger.setLogLevel(LogLevel::Trace);

    auto result = get_target_image();

    const auto& src = get_source_image();
    const auto& mask = get_mask();
    int offset_x = get_offset_x();
    int offset_y = get_offset_y();

    if (!matrix_precomputed_)
    {
        build_poisson_equation();
        precompute_matrix();
        matrix_precomputed_ = true;
    }

    for (int c = 0; c < 3; c++)
    {
        solve_channel(c);
    }
    return result;
}

}  // namespace USTC_CG
