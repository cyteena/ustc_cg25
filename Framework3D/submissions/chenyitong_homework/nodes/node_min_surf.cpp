#include "GCore/Components/MeshOperand.h"
#include "GCore/util_openmesh_bind.h"
#include "geom_node_base.h"
#include <cmath>
#include <time.h>
#include <Eigen/Sparse>

/*
** @brief HW4_TutteParameterization
**
** This file presents the basic framework of a "node", which processes inputs
** received from the left and outputs specific variables for downstream nodes to
** use.
** - In the first function, node_declare, you can set up the node's input and
** output variables.
** - The second function, node_exec is the execution part of the node, where we
** need to implement the node's functionality.
** Your task is to fill in the required logic at the specified locations
** within this template, especially in node_exec.
*/

NODE_DEF_OPEN_SCOPE
NODE_DECLARATION_FUNCTION(min_surf)
{
    // Input-1: Original 3D mesh with boundary
    b.add_input<Geometry>("Input");

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
    b.add_output<Geometry>("Output");

    // 添加可选的调试输出选项
    b.add_output<std::string>("DebugInfo", "");
}

NODE_EXECUTION_FUNCTION(min_surf)
{
    // Get the input from params
    auto input = params.get_input<Geometry>("Input");

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
        Eigen::VectorXd b_x = Eigen::VectorXd::Zero(n);
        Eigen::VectorXd b_y = Eigen::VectorXd::Zero(n);
        Eigen::VectorXd b_z = Eigen::VectorXd::Zero(n);

        for (auto v_it = halfedge_mesh->vertices_begin(); v_it != halfedge_mesh->vertices_end(); ++v_it)
        {
            int i = vertex_indices[*v_it];
            std::vector<typename OpenMesh::PolyMesh_ArrayKernelT<>::VertexHandle> neighbors;
            if (halfedge_mesh->is_boundary(*v_it))
            {
                // boundary points: set row to be 1.0, and right be the original coordinate
                triplets.emplace_back(T(i, i, 1.0));
                auto point = halfedge_mesh->point(*v_it);
                b_x(i) = point[0];
                b_y(i) = point[1];
                b_z(i) = point[2];
            }
            else
            {
                // internal points: right should be zero and set row to be laplacian
                int degree = 0;
                for (auto vv_it = halfedge_mesh->vv_iter(*v_it); vv_it.is_valid(); ++vv_it)
                {
                    int j = vertex_indices[*vv_it];
                    triplets.emplace_back(T(i, j, -1.0));
                    neighbors.emplace_back(*vv_it);
                    degree++;
                }
                triplets.emplace_back(T(i, i, degree));
            }
        }

        Eigen::SparseMatrix<double> L(n, n);
        L.setFromTriplets(triplets.begin(), triplets.end());

        // 求解线性系统
        Eigen::SparseLU<Eigen::SparseMatrix<double>> solver;
        solver.compute(L);

        // 检查分解是否成功
        if (solver.info() != Eigen::Success)
        {
            throw std::runtime_error("Minimal Surface: Failed to decompose Laplacian matrix.");
            return false;
        }

        Eigen::VectorXd x = solver.solve(b_x);
        Eigen::VectorXd y = solver.solve(b_y);
        Eigen::VectorXd z = solver.solve(b_z);

        // 检查求解是否成功
        if (solver.info() != Eigen::Success)
        {
            throw std::runtime_error("Minimal Surface: Failed to solve linear system.");
            return false;
        }

        // 4. 更新网格顶点位置
        for (auto v_it = halfedge_mesh->vertices_begin(); v_it != halfedge_mesh->vertices_end(); ++v_it)
        {
            int idx = vertex_indices[*v_it];
            halfedge_mesh->point(*v_it)[0] = x(idx);
            halfedge_mesh->point(*v_it)[1] = y(idx);
            halfedge_mesh->point(*v_it)[2] = z(idx);
        }

        // 确保输出有效
        auto geometry = openmesh_to_operand(halfedge_mesh.get());
        if (!geometry)
        {
            debug_info << "转换结果失败！";
            // 移除has_output检查，直接设置调试信息输出
            params.set_output("DebugInfo", debug_info.str());
            throw std::runtime_error("Minimal Surface: Failed to convert result to geometry.");
            return false;
        }

        debug_info << "处理成功，顶点数量: " << halfedge_mesh->n_vertices()
                   << ", 边界点数量: " << boundary_vertices.size();

        // 设置调试信息输出
        params.set_output("DebugInfo", debug_info.str());

        params.set_output("Output", std::move(*geometry));
        return true;
    }
    catch (const std::exception &e)
    {
        // 捕获任何异常并提供更详细的错误信息
        std::string error_msg = "Minimal Surface node error: ";
        error_msg += e.what();
        throw std::runtime_error(error_msg);
        return false;
    }
}

NODE_DECLARATION_UI(min_surf);
NODE_DEF_CLOSE_SCOPE
