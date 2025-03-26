#include "GCore/Components/MeshOperand.h"
#include "GCore/util_openmesh_bind.h"
#include "geom_node_base.h"
#include <cmath>
#include <Eigen/Sparse>

#include <pxr/base/gf/vec3f.h>
#include <pxr/base/vt/array.h>
#include <iostream> // 添加标准输出头文件

/*
** @brief HW5_Laplacian_Surface_Editing
**
** This file presents the basic framework of a "node", which processes inputs
** received from the left and outputs specific variables for downstream nodes to
** use.
**
** - In the first function, node_declare, you can set up the node's input and
** output variables.
**
** - The second function, node_exec is the execution part of the node, where we
** need to implement the node's functionality.
**
**
** Your task is to fill in the required logic at the specified locations
** within this template, especially in node_exec.
*/

NODE_DEF_OPEN_SCOPE
NODE_DECLARATION_FUNCTION(mesh_editing)
{
	// Input-1: Original 3D mesh with boundary
	// Input-2: Position of all points after change
	// Input-3: Indices of control points

	b.add_input<Geometry>("Original mesh");
	b.add_input<pxr::VtArray<pxr::GfVec3f>>("Changed vertices");
	b.add_input<std::vector<size_t>>("Control Points Indices");

	/*
	** NOTE: You can add more inputs or outputs if necessary. For example, in
	** some cases, additional information (e.g. other mesh geometry, other
	** parameters) is required to perform the computation.
	**
	** Be sure that the input/outputs do not share the same name. You can add
	 one geometry as
	**
	**                b.add_input<Geometry>("Input");
	**
	*/

	// Output: New positions of all points or changed mesh
	b.add_output<pxr::VtArray<pxr::GfVec3f>>("New vertices");
}

NODE_EXECUTION_FUNCTION(mesh_editing)
{
	// Get the input from params
	auto input = params.get_input<Geometry>("Original mesh");
	pxr::VtArray<pxr::GfVec3f> changed_vertices = params.get_input<pxr::VtArray<pxr::GfVec3f>>("Changed vertices");
	std::vector<size_t> control_points_indices = params.get_input<std::vector<size_t>>("Control Points Indices");

	// Avoid processing the node when there is no input
	if (!input.get_component<MeshComponent>())
	{
		std::cerr << "Mesh Editing: Need Geometry Input." << std::endl; // 使用标准错误输出
		return false;
	}
	if (control_points_indices.empty())
	{
		std::cerr << "Mesh Editing: At least one control point is required." << std::endl; // 使用标准错误输出
		return false;
	}

	auto mesh = operand_to_openmesh(&input);
	size_t n_vertices = mesh->n_vertices();

	// --- 1. 获取原始顶点位置 ---
	Eigen::MatrixXd V_orig(n_vertices, 3);
	for (auto vh : mesh->vertices()) {
		size_t i = vh.idx();
		auto p = mesh->point(vh);
		V_orig(i, 0) = p[0];
		V_orig(i, 1) = p[1];
		V_orig(i, 2) = p[2];
	}

	// 日志记录输入信息
	std::cout << "Starting mesh editing with "
			  << n_vertices
			  << " vertices and "
			  << control_points_indices.size()
			  << " control points" << std::endl;

	// 构建拉普拉斯矩阵
	std::cout << "Building Laplacian matrix for " << n_vertices << " vertices" << std::endl;
	Eigen::SparseMatrix<double> L(n_vertices, n_vertices);
	std::vector<Eigen::Triplet<double>> triplets;

	for (auto vh : mesh->vertices())
	{
		size_t i = vh.idx();
		double sum = 0.0;

		for (auto vv : mesh->vv_range(vh))
		{
			size_t j = vv.idx();
			triplets.emplace_back(i, j, -1.0);
			sum += 1.0;
		}
		triplets.emplace_back(i, i, sum);
	}
	L.setFromTriplets(triplets.begin(), triplets.end()); // the differentials delta = L V (V: the geometric position of mesh vertices) (L: D - A , A is the connectivity matrix)

	// --- 3. 计算原始拉普拉斯坐标 Delta = L * V_orig ---
	std::cout << "Calculating original Laplacian coordinates (Delta)" << std::endl;
	Eigen::MatrixXd Delta = L * V_orig; // n_vertices x 3 matrix


	// 组合矩阵
	std::cout << "Combining matrices for solving" << std::endl;
	Eigen::SparseMatrix<double> A(n_vertices + control_points_indices.size(), n_vertices);
	std::vector<Eigen::Triplet<double>> A_triplets;

	// 添加 L 矩阵部分
	for (int k = 0; k < L.outerSize(); ++k)
	{
		for (Eigen::SparseMatrix<double>::InnerIterator it(L, k); it; ++it)
		{
			A_triplets.emplace_back(it.row(), it.col(), it.value());
		}
	}

	// 添加 C 矩阵部分
	for (size_t i = 0; i < control_points_indices.size(); i++)
	{
		A_triplets.emplace_back(n_vertices + i, control_points_indices[i], 1.0);
	}

	A.setFromTriplets(A_triplets.begin(), A_triplets.end());

    // --- 5. 构建右侧向量 b = [Delta; P] ---
    std::cout << "Building right-hand side vector b" << std::endl;
    Eigen::MatrixXd b(n_vertices + control_points_indices.size(), 3);

	// 设置 Delta 部分 (前 n_vertices 行)
	b.topRows(n_vertices) = Delta;

	for (size_t i = 0; i < control_points_indices.size(); i++)
	{
		size_t idx = control_points_indices[i];
		b.row(n_vertices + i) = Eigen::Vector3d(
			changed_vertices[idx][0],
			changed_vertices[idx][1],
			changed_vertices[idx][2]);
	}

	// 求解
	std::cout << "Solving linear system" << std::endl;
	Eigen::LeastSquaresConjugateGradient<Eigen::SparseMatrix<double>> solver;
	solver.setTolerance(1e-6); // Set a tolerance
    solver.setMaxIterations(n_vertices * 2); // Set max iterations
	
	// 打印矩阵A的大小和部分内容
	std::cout << "Matrix A: " << A.rows() << " x " << A.cols() << std::endl;
	std::cout << "Number of non-zeros in A: " << A.nonZeros() << std::endl;
	if (A.rows() > 0 && A.cols() > 0)
	{
		std::cout << "First few non-zero elements in A:" << std::endl;
		int count = 0;
		for (int k = 0; k < A.outerSize() && count < 10; ++k)
		{
			for (Eigen::SparseMatrix<double>::InnerIterator it(A, k); it && count < 10; ++it)
			{
				std::cout << "(" << it.row() << ", " << it.col() << ") = " << it.value() << std::endl;
				count++;
			}
		}
	}

	// 调试输出b的内容
	std::cout << "Vector b info:\n"
			  << "Rows: " << b.rows() << ", Cols: " << b.cols() << std::endl;
	std::cout << "Vector b (last " << control_points_indices.size() << " rows):" << std::endl;
	for (size_t i = 0; i < control_points_indices.size(); i++)
	{
		std::cout << "b[" << n_vertices + i << "] = ("
				  << b(n_vertices + i, 0) << ", "
				  << b(n_vertices + i, 1) << ", "
				  << b(n_vertices + i, 2) << ")" << std::endl;
	}

	// 修改后的求解部分
	std::cout << "Attempting matrix decomposition..." << std::endl;
	// 添加矩阵验证
	auto verify_matrix = [](const Eigen::SparseMatrix<double> &mat)
	{
		for (int k = 0; k < mat.outerSize(); ++k)
		{
			for (Eigen::SparseMatrix<double>::InnerIterator it(mat, k); it; ++it)
			{
				if (!std::isfinite(it.value()))
				{
					std::cerr << "Invalid matrix value at ("
							  << it.row() << "," << it.col() << "): "
							  << it.value() << std::endl;
					return false;
				}
			}
		}
		return true;
	};

	if (!verify_matrix(A))
	{
		throw std::runtime_error("Matrix contains invalid values");
	}
	solver.compute(A);

	

    // 2. 求解系统
    std::cout << "Solving system column by column..." << std::endl;
    Eigen::MatrixXd X(n_vertices, 3);
    
    // 对每一列单独求解
    for (int col = 0; col < 3; ++col) {
        std::cout << "Solving for column " << col << "..." << std::endl;
        Eigen::VectorXd b_col = b.col(col);
        X.col(col) = solver.solve(b_col);
        if (solver.info() != Eigen::Success) {
            std::cerr << "System solving failed for column " << col 
                      << " with error code: " << solver.info() << std::endl;
            throw std::runtime_error("System solving failed for column " + std::to_string(col));
        }
    }
    
    std::cout << "Successfully solved AX = b for all columns" << std::endl;

	// 3. 输出结果
	std::cout << "Solution found with min coeff: " << X.minCoeff()
			  << ", max coeff: " << X.maxCoeff() << std::endl;

	pxr::VtArray<pxr::GfVec3f> new_positions(n_vertices);
	for (size_t i = 0; i < n_vertices; i++)
	{
		new_positions[i] = pxr::GfVec3f(
			static_cast<double>(X(i, 0)),
			static_cast<double>(X(i, 1)),
			static_cast<double>(X(i, 2)));
	}
	params.set_output("New vertices", std::move(new_positions));
}

NODE_DECLARATION_UI(mesh_editing);
NODE_DEF_CLOSE_SCOPE