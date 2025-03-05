#include "NN_warper.h"
#include <cmath>
#include <iostream>
#include <algorithm>

namespace USTC_CG
{

NNWarper::NNWarper(
    const std::vector<ImVec2>& start_points,
    const std::vector<ImVec2>& end_points)
    : start_points_(start_points),
      end_points_(end_points)
{
    if (start_points.empty() || start_points.size() != end_points.size()) {
        throw std::runtime_error("Invalid input points");
    }

    // 计算数据范围，用于归一化
    float min_x = start_points[0].x, max_x = start_points[0].x;
    float min_y = start_points[0].y, max_y = start_points[0].y;
    
    for (const auto& p : start_points) {
        min_x = std::min(min_x, p.x);
        max_x = std::max(max_x, p.x);
        min_y = std::min(min_y, p.y);
        max_y = std::max(max_y, p.y);
    }
    
    // 存储归一化参数，用于后续推断
    x_scale_ = max_x - min_x > 1e-6f ? 2.0f / (max_x - min_x) : 1.0f;
    y_scale_ = max_y - min_y > 1e-6f ? 2.0f / (max_y - min_y) : 1.0f;
    x_offset_ = min_x;
    y_offset_ = min_y;

    // 准备训练数据（归一化后）
    std::vector<dlib::matrix<float>> inputs, targets;
    inputs.reserve(start_points.size());
    targets.reserve(end_points.size());

    for (size_t i = 0; i < start_points.size(); i++)
    {
        // 归一化输入数据到 [-1, 1] 范围
        dlib::matrix<float> input(2, 1);
        input(0, 0) = (start_points[i].x - x_offset_) * x_scale_ - 1.0f;
        input(1, 0) = (start_points[i].y - y_offset_) * y_scale_ - 1.0f;
        inputs.push_back(input);

        // 同样归一化输出数据
        dlib::matrix<float> target(2, 1);
        target(0, 0) = (end_points[i].x - x_offset_) * x_scale_ - 1.0f;
        target(1, 0) = (end_points[i].y - y_offset_) * y_scale_ - 1.0f;
        targets.push_back(target);
    }

    // 配置训练器
    dlib::dnn_trainer<net_type> trainer(net_);
    trainer.set_learning_rate(0.01);
    trainer.set_min_learning_rate(1e-6);
    trainer.set_mini_batch_size(std::min(32ul, static_cast<unsigned long>(start_points.size())));
    
    
    // 关闭过于详细的输出，避免日志过于冗长
    trainer.be_verbose();
    
    // // 设置合适的同步间隔
    // trainer.set_synchronization_file("nn_warper_sync", std::chrono::seconds(30));
    
    trainer.train(inputs, targets);
}

std::pair<float, float> NNWarper::warp(float x, float y)
{
    try {
        // 归一化输入
        dlib::matrix<float> input(2, 1);
        input(0, 0) = (x - x_offset_) * x_scale_ - 1.0f;
        input(1, 0) = (y - y_offset_) * y_scale_ - 1.0f;

        // 网络前向传播
        auto output = net_(input);
        
        // 反归一化输出
        float out_x = (output(0, 0) + 1.0f) / x_scale_ + x_offset_;
        float out_y = (output(1, 0) + 1.0f) / y_scale_ + y_offset_;
        
        return {out_x, out_y};
    }
    catch (const std::exception& e) {
        std::cerr << "变形错误: " << e.what() << std::endl;
        return {x, y}; // 出错时返回原坐标
    }
}

}  // namespace USTC_CG