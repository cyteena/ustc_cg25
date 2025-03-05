#include "RBF_warper.h"
#include <cmath>
#include <Eigen/Dense>

namespace USTC_CG
{
RBFWarper::RBFWarper(
    const std::vector<ImVec2>& start_points,
    const std::vector<ImVec2>& end_points)
    : start_points_(start_points),
      end_points_(end_points)
{
    using namespace Eigen;
    const auto n = start_points.size();
    if (n != end_points.size() || n < 1)
    {
        A_[0][0] = 1.0f; A_[0][1] = 0.0f;
        A_[1][0] = 0.0f; A_[1][1] = 1.0f;
        b_.x = 0.0f; b_.y = 0.0f;
        return;
    }

    // 构建组合矩阵 K = [[R, P]; [P^T, 0]]
    MatrixXf K(n + 3, n + 3);
    K.setZero();
    
    // 填充RBF核矩阵部分（添加极小值防止除零）
    constexpr float eps = 1e-9f;
    for (int i = 0; i < static_cast<int>(n); ++i) {
        for (int j = 0; j < static_cast<int>(n); ++j) {
            const float dx = start_points[i].x - start_points[j].x;
            const float dy = start_points[i].y - start_points[j].y;
            const float r_sq = dx*dx + dy*dy;
            const float r = sqrt(r_sq);
            K(i, j) = r_sq * log(r + eps);  // 添加极小值保护
        }
    }

    // 填充仿射约束部分
    for (int i = 0; i < static_cast<int>(n); ++i) {
        K(i, n) = start_points[i].x;    // a11
        K(i, n+1) = start_points[i].y;  // a12
        K(i, n+2) = 1.0f;               // b1
        K(n, i) = start_points[i].x;
        K(n+1, i) = start_points[i].y;
        K(n+2, i) = 1.0f;
    }

    // 构建右侧向量 [q_x; 0] 和 [q_y; 0]
    VectorXf Vx(n + 3), Vy(n + 3);
    for (int i = 0; i < static_cast<int>(n); ++i) {
        Vx(i) = end_points[i].x  - start_points[i].x; // 计算位移量
        Vy(i) = end_points[i].y - start_points[i].y;
    }
    Vx.tail<3>().setZero();
    Vy.tail<3>().setZero();

    // 4. 求解系统（使用QR分解更稳定）
    ColPivHouseholderQR<MatrixXf> qr(K);
    VectorXf alpha_x = qr.solve(Vx);
    VectorXf alpha_y = qr.solve(Vy);

    // 存储结果
    alpha_x_.resize(n);
    alpha_y_.resize(n);
    for (int i = 0; i < static_cast<int>(n); ++i) {
        alpha_x_[i] = alpha_x(i);
        alpha_y_[i] = alpha_y(i);
    }
    
    // 提取仿射变换参数（修正二维数组赋值方式）
    A_[0][0] = alpha_x(n);
    A_[0][1] = alpha_x(n+1);
    A_[1][0] = alpha_y(n);
    A_[1][1] = alpha_y(n+1);
    b_.x = alpha_x(n+2);
    b_.y = alpha_y(n+2);

}

std::pair<float, float> RBFWarper::warp(float x, float y) 
{
    // 将错误放置在构造函数中的变形代码移回此处
    constexpr float eps = 1e-8f;
    float result_x = A_[0][0] * x + A_[0][1] * y + b_.x + x;
    float result_y = A_[1][0] * x + A_[1][1] * y + b_.y + y;

    for (size_t i = 0; i < start_points_.size(); ++i) {
        const float dx = x - start_points_[i].x;
        const float dy = y - start_points_[i].y;
        const float r_sq = dx*dx + dy*dy + eps;
        const float r = sqrt(r_sq);
        const float phi = r_sq * log(r + eps);
        
        result_x += alpha_x_[i] * phi;
        result_y += alpha_y_[i] * phi;
    }

    return {result_x, result_y};
}
}