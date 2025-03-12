#include "Seamless.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

#include "Log.h"

namespace USTC_CG
{
Logger logger;
using uchar = unsigned char;

std::shared_ptr<Image> Seamless::solve()
{
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

void Seamless::solve_channel(int channel)
{
    const int N = A_.rows();
    logger.debug() << "Solving channel " << channel << " with " << N
                   << " variables" << std::endl;

    if (solver_.info() != Eigen::Success)
    {
        logger.error() << "Solver not initialized properly for channel "
                       << channel;
        throw std::runtime_error("Solver not ready");
    }

    Eigen::VectorXd x = solver_.solve(b_.col(channel));
    if (solver_.info() != Eigen::Success)
    {
        logger.error() << "Failed to solve channel " << channel
                       << ", error code: " << solver_.info();
        throw std::runtime_error("Linear system solve failed");
    }

    // 获取目标图像引用避免重复调用
    auto result = get_target_image();
    const int target_width = result->width();
    const int target_height = result->height();
    const auto& mask = get_mask();
    const int width = mask->width();
    const int height = mask->height();

    int valid_count = 0;
    int out_of_bound = 0;

    for (const auto& [pos, i] : index_map)
    {
        const int pixel_x = pos % width;
        const int pixel_y = pos / width;

        // 计算目标坐标（带偏移量）
        const int target_x = pixel_x + get_offset_x();
        const int target_y = pixel_y + get_offset_y();

        // 边界检查
        if (target_x >= 0 && target_x < target_width && target_y >= 0 &&
            target_y < target_height)
        {
            auto pixel = result->get_pixel(target_x, target_y);
            const double solved_value = x[i];

            // 带溢出保护的数值转换
            pixel[channel] =
                static_cast<uchar>(std::clamp(solved_value, 0.0, 255.0));

            result->set_pixel(target_x, target_y, pixel);
            valid_count++;
        }
        else
        {
            out_of_bound++;
        }
    }

    // 添加解的质量报告
    logger.debug() << "Channel " << channel << " solution stats:\n"
                   << "  Valid pixels: " << valid_count << "\n"
                   << "  Out-of-bound: " << out_of_bound << "\n"
                   << "  Value range: [" << x.minCoeff() << ", " << x.maxCoeff()
                   << "]" << std::endl;
}

void Seamless::build_poisson_equation()
{
    const auto& src = get_source_image();
    const auto& mask = get_mask();
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
                        b_(i, c) += get_target_image()->get_pixel(
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
            b_(i, c) += neighbor_count * src_val;
            for (const auto& [nx, ny] : neighbors)
            {
                if (nx >= 0 && nx < width && ny >= 0 && ny < height)
                {
                    b_(i, c) -= src->get_pixel(nx, ny)[c];
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
}

// 矩阵构建完成后增加总结日志

void Seamless::precompute_matrix()
{
    logger.debug() << "Starting matrix decomposition (LDLT)...";

    if (A_.rows() == 0)
    {
        logger.error() << "Poisson equation matrix is empty!";
        throw std::runtime_error("Empty coefficient matrix");
    }

    try
    {
        solver_.compute(A_);

        if (solver_.info() != Eigen::Success)
        {
            logger.error() << "Matrix decomposition failed with error code: "
                           << solver_.info();
            throw std::runtime_error("Matrix decomposition failed");
        }

        logger.info() << "Matrix decomposed successfully. Non-zero elements: "
                      << A_.nonZeros();
    }
    catch (const std::exception& e)
    {
        logger.error() << "Matrix decomposition error: " << e.what();
        throw;
    }
}

}  // namespace USTC_CG

// namespace USTC_CG
