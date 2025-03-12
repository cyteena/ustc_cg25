#pragma once

#include "Eigen/Sparse"
#include "clonemethod.h"
#include "common/image_widget.h"

namespace USTC_CG
{
class Seamless : public CloneMethod
{
   public:
    Seamless(
        std::shared_ptr<Image> src,
        std::shared_ptr<Image> dst,
        std::shared_ptr<Image> mask,
        int offset_x,
        int offset_y)
        : CloneMethod(src, dst, mask, offset_x, offset_y)
    {
    }

    std::shared_ptr<Image> solve() override;

   protected:
    virtual void build_poisson_equation();

    void precompute_matrix();

    void solve_channel(int channel);

    // LDLT: lower Diagonal lower transpose (suitable for symmetrix
    // positive-definite sparse) decomposes the matrix into a product of a lower
    // triangular matrix (L), a diagonal matrix (D) and L^T
    Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> solver_;

    // cache the decomposition of the matrix
    bool matrix_precomputed_ = false;
    Eigen::SparseMatrix<double> A_;
    Eigen::MatrixXd b_;  // the right vector of possion equation
    std::unordered_map<int, int> index_map;
};
}  // namespace USTC_CG
