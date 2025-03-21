#include "GCore/Components/MeshOperand.h"
#include "GCore/util_openmesh_bind.h"
#include "geom_node_base.h"
#include <cmath>
#include <time.h>
#include <Eigen/Sparse>

namespace OpenMeshUtils
{

    template <typename MeshT>
    double cotangent_weight(const MeshT &mesh,
                            typename MeshT::HalfedgeHandle he)
    {

        if (!mesh.is_valid_handle(he))
            return 0.0;

        // 获取三个顶点坐标
        const auto v0 = mesh.point(mesh.from_vertex_handle(he));
        const auto v1 = mesh.point(mesh.to_vertex_handle(he));
        // 正确获取对面顶点
        const auto next_he = mesh.next_halfedge_handle(he);
        if (!mesh.is_valid_handle(next_he))
            return 0.0;

        const auto v2 = mesh.point(mesh.to_vertex_handle(next_he));

        // 计算两个边向量
        OpenMesh::Vec3f e1, e2;
        e1 = v0 - v2;
        e2 = v1 - v2;

        // 计算点积和叉乘模长
        const auto dot = OpenMesh::dot(e1, e2);
        const auto cross = OpenMesh::cross(e1, e2).norm();
        const auto cond_max = 1e6;

        // 防止除零
        return (cross < 1e-8) ? 1e6 : (dot / cross);
    }

} // namespace OpenMeshUtils

NODE_DEF_OPEN_SCOPE
NODE_DECLARATION_FUNCTION(compute_cotangent_weights)
{
    // Input-1: Original 3D mesh with boundary
    b.add_input<Geometry>("InputMesh");
    b.add_output<Eigen::SparseMatrix<double>>("Cotangent_weights");
    /*
    ** NOTE: You can add more inputs or outputs if necessary. For example, in
    *some cases,
    ** additional information (e.g. other mesh geometry, other parameters) is
    *required to perform
    ** the computation.
    **
    ** Be sure that the input/outputs do not share the same name. You can add
    *one geometry as
    **
    **                b.add_input<Geometry>("Input");
    **
    ** Or maybe you need a value buffer like:
    **
    **                b.add_input<float1Buffer>("Weights");
    */

    // Output-1: Minimal surface with fixed boundary
}

NODE_EXECUTION_FUNCTION(compute_cotangent_weights)
{
    // Get the input from params
    auto input = params.get_input<Geometry>("Input");

    log::info("Is reading input to compute cotangent weights");

    // 增强输入检查
    if (!input.get_component<MeshComponent>())
    {
        throw std::runtime_error("Minimal Surface: Need Geometry Input.");
        return false;
    }

    try
    {
        // 输出调试信息
        std::stringstream debug_info;
        debug_info << "正在处理最小曲面... ";

        // 创建halvede结构
        auto halfedge_mesh = operand_to_openmesh(&input);

        // 检查网格是否有效
        if (!halfedge_mesh || halfedge_mesh->n_vertices() == 0)
        {
            throw std::runtime_error("Minimal Surface: Invalid mesh input (no vertices).");
            return false;
        }

        // 1. 创建顶点索引映射并收集边界/内部顶点
        std::vector<int> boundary_vertices;
        std::vector<int> internal_vertices;
        std::map<typename OpenMesh::PolyMesh_ArrayKernelT<>::VertexHandle, int> vertex_indices;

        int index = 0;
        for (auto v_it = halfedge_mesh->vertices_begin(); v_it != halfedge_mesh->vertices_end(); ++v_it)
        {
            vertex_indices[*v_it] = index++;
            if (halfedge_mesh->is_boundary(*v_it))
            {
                boundary_vertices.emplace_back(vertex_indices[*v_it]);
            }
            else
            {
                internal_vertices.emplace_back(vertex_indices[*v_it]);
            }
        }

        // 检查边界顶点
        if (boundary_vertices.empty())
        {
            throw std::runtime_error("Minimal Surface: No boundary vertices found in mesh.");
            return false;
        }

        // 2. 构建拉普拉斯矩阵
        typedef Eigen::Triplet<double> T;
        std::vector<T> triplets;
        int n = halfedge_mesh->n_vertices();

        for (auto v_it = halfedge_mesh->vertices_begin(); v_it != halfedge_mesh->vertices_end(); ++v_it)
        {
            int i = vertex_indices[*v_it];
            std::vector<typename OpenMesh::PolyMesh_ArrayKernelT<>::VertexHandle> neighbors;
            if (halfedge_mesh->is_boundary(*v_it))
            {
                // boundary points: set row to be 1.0, and right be the original coordinate
                triplets.emplace_back(T(i, i, 1.0));
            }
            else
            {
                // internal points: right should be zero and set row to be laplacian
                double weight_sum = 0.0;
                for (auto vv_it = halfedge_mesh->vv_iter(*v_it); vv_it.is_valid(); ++vv_it)
                {
                    int j = vertex_indices[*vv_it];
                    double weight = 0.0;
                    auto he = halfedge_mesh->find_halfedge(*v_it, *vv_it);

                    // get the adjacent face
                    if (he.is_valid())
                    {
                        debug_info << "处理顶点 " << i << " 和邻居 " << j << " 的连接 ";

                        // forward face
                        if (halfedge_mesh->face_handle(he).is_valid())
                        {
                            double cot_weight = OpenMeshUtils::cotangent_weight(*halfedge_mesh, he);
                            weight += cot_weight;
                            debug_info << "(正向权重: " << cot_weight << ") ";
                        }
                        else
                        {
                            debug_info << "(无正向面) ";
                        }

                        // 反向面
                        auto opp_he = halfedge_mesh->opposite_halfedge_handle(he);
                        if (opp_he.is_valid() && halfedge_mesh->face_handle(opp_he).is_valid())
                        {
                            double cot_weight = OpenMeshUtils::cotangent_weight(*halfedge_mesh, opp_he);
                            weight += cot_weight;
                            debug_info << "(反向权重: " << cot_weight << ") ";
                        }
                        else
                        {
                            debug_info << "(无有效反向半边) ";
                            throw std::runtime_error("Invalid halfedge between vertices");
                        }

                        debug_info << "总权重: " << weight << std::endl;
                    }
                    triplets.emplace_back(i, j, -weight);
                    weight_sum += weight;
                }
                triplets.emplace_back(T(i, i, weight_sum));
            }
        }

        Eigen::SparseMatrix<double> L(n, n);
        L.setFromTriplets(triplets.begin(), triplets.end());

        params.set_output("Cotangent_weights", L);
        // 求解线性系统
    }
    catch (const std::exception &e)
    {
        // 捕获任何异常并提供更详细的错误信息
        std::string error_msg = "Cotangent_weights Error: ";
        error_msg += e.what();
        throw std::runtime_error(error_msg);
        return false;
    }
}

NODE_DECLARATION_UI(compute_cotangent_weights);
NODE_DEF_CLOSE_SCOPE
