#include "GCore/Components/MeshOperand.h" // 假设的 GCore 头文件
#include "GCore/util_openmesh_bind.h"	  // 假设的 GCore 头文件
#include "geom_node_base.h"				  // 假设的 GCore 头文件 (节点基类)
#include <cmath>
#include <vector>
#include <functional> // 用于 std::hash
#include <limits>	  // 用于 std::numeric_limits
#include <iostream>	  // 标准输出头文件
#include <stdexcept>  // 用于 std::runtime_error
#include "io/json.hpp"
#include <map>

// Eigen 库头文件
#include <Eigen/Sparse>
#include <Eigen/SparseQR> // 包含 SparseQR 求解器

// USD/PXR 库头文件 (用于数据类型)
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/vt/array.h>

// OpenMesh 库头文件 (如果 operand_to_openmesh 需要)
// #include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh> // 具体类型取决于你的定义

/*
** @brief HW5_Laplacian_Surface_Editing
** (原始注释)
*/
namespace OpenMeshUtils
{
    // Calculates the cotangent of the angle opposite the given halfedge 'he'
    // within the face attached to 'he'.
    template <typename MeshT>
    double cotangent_weight(const MeshT &mesh, typename MeshT::HalfedgeHandle he)
    {
        // Check if the halfedge and its face are valid
        if (!mesh.is_valid_handle(he) || mesh.is_boundary(he)) {
            return 0.0; // No valid opposite angle for boundary halfedges
        }

        // Get the vertices: v0 -> v1 is the edge itself, v2 is the opposite vertex
        const auto v0h = mesh.from_vertex_handle(he);
        const auto v1h = mesh.to_vertex_handle(he);
        const auto v2h = mesh.to_vertex_handle(mesh.next_halfedge_handle(he)); // Vertex opposite he

        if (!mesh.is_valid_handle(v0h) || !mesh.is_valid_handle(v1h) || !mesh.is_valid_handle(v2h)) {
             return 0.0; // Invalid vertex handle encountered
        }

        const auto v0 = mesh.point(v0h);
        const auto v1 = mesh.point(v1h);
        const auto v2 = mesh.point(v2h);

        // Calculate the two edge vectors originating from the opposite vertex (v2)
        // Use OpenMesh vector type consistent with the function's operations
        const typename MeshT::Point e1 = v0 - v2; // Vector from v2 to v0
        const typename MeshT::Point e2 = v1 - v2; // Vector from v2 to v1

        // Calculate dot product and the norm of the cross product
        const double dot_product = OpenMesh::dot(e1, e2);
        // Use double for cross product norm to maintain precision for division
        const double cross_product_norm = OpenMesh::cross(e1, e2).norm();

        // Avoid division by zero or near-zero for degenerate triangles
        const double epsilon = 1e-9; // Use a small epsilon
        if (cross_product_norm < epsilon) {
            // Consider degenerate triangles as having infinite cotangent (or a very large weight).
            // Returning 0 might be safer in practice to avoid extreme weights. Choose one.
             // return 1.0 / epsilon; // Very large weight (original logic seemed to imply this)
             return 0.0; // Safer alternative: ignore degenerate triangles for weight calculation
        }

        // Cotangent = dot(e1, e2) / |cross(e1, e2)|
        return dot_product / cross_product_norm;
    }

} // namespace OpenMeshUtils


enum class LaplacianWeightType {
	UNIFORM,
	COTANGENT
};

// --- 定义用于存储预计算结果的结构体 ---
// 这个结构体将持久化存储在节点实例中 (通过 GCore 框架)

// --- Definition for SolverStorage ---
struct SolverStorage {
    // --- Framework Integration ---
    constexpr static bool has_storage = true;

    // --- Core Solver Data (Not serialized) ---
    Eigen::SparseQR<Eigen::SparseMatrix<double>, Eigen::COLAMDOrdering<int>> solver;
    Eigen::MatrixXd Delta;

    // --- State Variables (Will be serialized to JSON) ---
    size_t num_vertices = 0;
    size_t control_indices_hash = 0;
    double constraint_weight = 1.0;
	bool use_cotangent_weights = false; // <<< Store the type used for last compute (true=Cotangent, false=Uniform)
    bool solver_ready = false; // Status flag (reset by deserialize)

    // --- Serialization Interface using nlohmann::json ---

    /**
     * @brief Deserializes state from a nlohmann::json object.
     * @param storage_info The JSON object containing the serialized state.
     * @return true if deserialization was successful (or handled), false on error.
     */
    bool deserialize(const nlohmann::json& storage_info) { // Take json by const reference
        std::cout << "DEBUG: SolverStorage::deserialize(const nlohmann::json&) called." << std::endl;
        try {
            // Check if keys exist before accessing (safer)
            if (storage_info.contains("num_vertices")) {
                num_vertices = storage_info.value("num_vertices", (size_t)0); // Provide default value
            } else {
                num_vertices = 0; // Default if key missing
                std::cout << "  Warning: 'num_vertices' key missing in saved JSON state." << std::endl;
            }

            if (storage_info.contains("control_indices_hash")) {
                control_indices_hash = storage_info.value("control_indices_hash", (size_t)0);
            } else {
                control_indices_hash = 0;
                std::cout << "  Warning: 'control_indices_hash' key missing in saved JSON state." << std::endl;
            }

            if (storage_info.contains("constraint_weight")) {
                constraint_weight = storage_info.value("constraint_weight", 1.0);
            } else {
                constraint_weight = 1.0; // Default weight
                std::cout << "  Warning: 'constraint_weight' key missing in saved JSON state." << std::endl;
            }

             std::cout << "  Deserialized: num_vertices=" << num_vertices
                       << ", hash=" << control_indices_hash
                       << ", weight=" << constraint_weight << std::endl;

        } catch (const nlohmann::json::exception& e) {
            // Handle potential errors during JSON access (e.g., type mismatch)
            std::cerr << "Error during SolverStorage JSON deserialize: " << e.what() << std::endl;
            // Reset state to default on error?
            num_vertices = 0;
            control_indices_hash = 0;
            constraint_weight = 1.0;
            solver_ready = false; // Ensure recompute on error
            return false; // Indicate failure
        } catch (const std::exception& e) {
            // Handle other potential standard exceptions
            std::cerr << "Standard error during SolverStorage deserialize: " << e.what() << std::endl;
            solver_ready = false;
            return false;
        }

        // CRITICAL: Always force recompute after loading state, regardless of success
        solver_ready = false;
        std::cout << "  SolverStorage deserialized, solver_ready forced to false." << std::endl;
        return true; // Indicate success (or that loading was handled)
    }

    /**
     * @brief Serializes state into a nlohmann::json object.
     * @param storage_info The JSON object to write the state into.
     *                     Passed by non-const ref because we modify it.
     */
    void serialize(nlohmann::json& storage_info) const { // json passed by non-const ref
        std::cout << "DEBUG: SolverStorage::serialize(nlohmann::json&) const called." << std::endl;
        try {
            // Write state variables as key-value pairs into the JSON object
            storage_info["num_vertices"] = num_vertices;
            storage_info["control_indices_hash"] = control_indices_hash;
            storage_info["constraint_weight"] = constraint_weight;

            // DO NOT serialize 'solver' or 'Delta'
            // DO NOT typically serialize 'solver_ready' - rely on deserialize to reset it.

            std::cout << "  Serialized: num_vertices=" << num_vertices
                       << ", hash=" << control_indices_hash
                       << ", weight=" << constraint_weight << std::endl;

        } catch (const nlohmann::json::exception& e) {
            // Handle potential errors during JSON construction/assignment
            std::cerr << "Error during SolverStorage JSON serialize: " << e.what() << std::endl;
            // Optionally clear storage_info on error?
            // storage_info = nlohmann::json{};
        } catch (const std::exception& e) {
             std::cerr << "Standard error during SolverStorage serialize: " << e.what() << std::endl;
        }
    }
}; // End SolverStorage

// --- 辅助函数：检查稀疏矩阵中是否有无效值 (NaN/Inf) ---
bool verify_matrix_is_finite(const Eigen::SparseMatrix<double> &mat)
{
	for (int k = 0; k < mat.outerSize(); ++k)
	{
		for (Eigen::SparseMatrix<double>::InnerIterator it(mat, k); it; ++it)
		{
			if (!std::isfinite(it.value()))
			{
				std::cerr << "ERROR: Invalid value in sparse matrix at ("
						  << it.row() << ", " << it.col() << "): " << it.value() << std::endl;
				return false;
			}
		}
	}
	return true;
}

NODE_DEF_OPEN_SCOPE // GCore 框架宏

// --- 节点输入/输出声明 ---
NODE_DECLARATION_FUNCTION(mesh_editing)
{
	// Input-1: 原始网格
	b.add_input<Geometry>("Original mesh");
	// Input-2: 改变后的顶点位置 (包含所有顶点，但我们主要关心控制点的新位置)
	b.add_input<pxr::VtArray<pxr::GfVec3f>>("Changed vertices");
	// Input-3: 控制点的索引列表
	b.add_input<std::vector<size_t>>("Control Points Indices");
	// Input-4: UNIFORM OR COTANGENT
	b.add_input<bool>("Use Cotangent Weights").default_val(false);

	// Optional: 可以添加约束权重作为可调节的输入参数
	// b.add_input<double>("Constraint Weight", 1.0);

	// Output: 计算出的所有顶点的新位置数组
	b.add_output<pxr::VtArray<pxr::GfVec3f>>("New vertices");
}

// --- 节点执行逻辑 ---
// 这个函数会在输入变化时被 GCore 框架调用
NODE_EXECUTION_FUNCTION(mesh_editing)
{
	// --- 1. 获取输入参数 ---
	auto input_geom = params.get_input<Geometry>("Original mesh");
	pxr::VtArray<pxr::GfVec3f> changed_vertices = params.get_input<pxr::VtArray<pxr::GfVec3f>>("Changed vertices");
	std::vector<size_t> control_points_indices = params.get_input<std::vector<size_t>>("Control Points Indices");
	bool use_cotangent_input = params.get_input<bool>("Use Cotangent Weights");
	// double current_constraint_weight = params.get_input<double>("Constraint Weight"); // 如果添加了权重输入

	// --- 2. 访问/获取持久化存储 ---
	// 从框架获取与此节点实例关联的 SolverStorage 对象
	// 如果是第一次执行或加载后，框架会创建一个新的或加载保存的状态
	// !! 确认 params.get_storage<SolverStorage&>() 是正确的用法 !!
	SolverStorage &storage = params.get_storage<SolverStorage &>();

	// --- 3. 输入有效性检查 ---
	if (!input_geom.get_component<MeshComponent>())
	{
		std::cerr << "Mesh Editing Error: Input 'Original mesh' is missing or invalid." << std::endl;
		return false; // 返回 false 表示节点执行失败
	}
	if (control_points_indices.empty())
	{
		std::cerr << "Mesh Editing Error: Input 'Control Points Indices' is empty. At least one control point is required." << std::endl;
		return false;
	}

	// --- 4. 获取网格信息 ---
	// 将 GCore::Geometry 转换为 OpenMesh 对象 (假设函数可用)
	auto mesh = operand_to_openmesh(&input_geom);
	if (!mesh || mesh->n_vertices() == 0)
	{
		std::cerr << "Mesh Editing Error: Failed to convert input to valid OpenMesh or mesh is empty." << std::endl;
		return false;
	}
	size_t n_vertices = mesh->n_vertices();
	size_t n_control_points = control_points_indices.size();

	// 详细检查输入数组和索引
	for (size_t idx : control_points_indices)
	{
		if (idx >= n_vertices)
		{
			std::cerr << "Mesh Editing Error: Control point index " << idx << " is out of bounds (>= num_vertices " << n_vertices << ")." << std::endl;
			return false;
		}
	}
	// 检查 changed_vertices 是否至少包含所有顶点（或者至少包含最大索引的控制点）
	if (changed_vertices.empty() || changed_vertices.size() < n_vertices)
	{
		// 根据你的场景，这可能是一个警告或一个错误
		std::cerr << "Mesh Editing Warning/Error: Size of 'Changed vertices' (" << changed_vertices.size()
				  << ") is smaller than mesh vertex count (" << n_vertices << "). Ensure it contains data for all control points." << std::endl;
		// 如果控制点可能引用超出 changed_vertices 范围的索引，则应返回错误
		// return false;
	}

    std::cout << "Mesh Editing Node: " << n_vertices << " vertices, " << n_control_points << " controls. Requested weights: "
              << (use_cotangent_input ? "Cotangent" : "Uniform") << std::endl;

	// --- 5. 检查是否需要重新进行预计算 ---
	//    预计算包括构建 L, A, Delta 并对 A 进行 QR 分解，这是最耗时的部分
	bool needs_recompute = false;
	size_t current_indices_hash = 0;
	// 假设权重固定为 1.0，如果可调则从输入或 storage 获取
	double current_constraint_weight = 1.0;

	// a) 顶点数量是否与上次计算时不同? 或者求解器从未成功初始化 (例如第一次运行或加载后)?
	if (n_vertices != storage.num_vertices || !storage.solver_ready)
	{
		needs_recompute = true;
		if (!storage.solver_ready)
		{
			std::cout << "  Recompute Trigger: Solver not ready (initial run or after load)." << std::endl;
		}
		else
		{
			std::cout << "  Recompute Trigger: Vertex count changed (" << storage.num_vertices << " -> " << n_vertices << ")." << std::endl;
		}
	}

	// b) 控制点索引集合是否改变? (仅当顶点数未变时检查)
	if (!needs_recompute)
	{
		std::hash<size_t> hasher;
		current_indices_hash = n_control_points; // 用数量初始化哈希
		for (size_t idx : control_points_indices)
		{
			// 结合哈希值 (一种常用的组合方式，避免冲突)
			current_indices_hash ^= hasher(idx) + 0x9e3779b9 + (current_indices_hash << 6) + (current_indices_hash >> 2);
		}
		if (current_indices_hash != storage.control_indices_hash)
		{
			needs_recompute = true;
			std::cout << "  Recompute Trigger: Control points set changed (hash mismatch)." << std::endl;
		}
	}

	// c) Weight type selection change? <<< New Check
	if (!needs_recompute && use_cotangent_input != storage.use_cotangent_weights) {
		needs_recompute = true;
		std::cout << "  Recompute Trigger: Weight type changed (User: " << use_cotangent_input
					<< ", Stored: " << storage.use_cotangent_weights << ")." << std::endl;
	}

	// d) 约束权重是否改变? (如果权重是可调参数)
	// if (!needs_recompute && std::abs(current_constraint_weight - storage.constraint_weight) > std::numeric_limits<double>::epsilon()) {
	//     needs_recompute = true;
	//     std::cout << "  Recompute Trigger: Constraint weight changed." << std::endl;
	// }

	// --- 6. 如果需要，执行预计算步骤 ---
	//    (构建 L, Delta, A; 计算 QR 分解)
	if (needs_recompute)
	{
		std::cout << "Performing full solver precomputation QR (Weights: "
                  << (use_cotangent_input ? "Cotangent" : "Uniform") << ")..." << std::endl;
		storage.solver_ready = false; // 在成功完成前标记为未就绪

		// --- a) 获取原始顶点位置 V_orig (使用 double 提高精度) ---
		Eigen::MatrixXd V_orig(n_vertices, 3);
		for (auto vh : mesh->vertices())
		{
			size_t i = vh.idx();
			auto p = mesh->point(vh);				  // 获取 OpenMesh 点坐标
			V_orig(i, 0) = static_cast<double>(p[0]); // 转换为 double
			V_orig(i, 1) = static_cast<double>(p[1]);
			V_orig(i, 2) = static_cast<double>(p[2]);
		}

		// --- b) 构建拉普拉斯矩阵 L (均匀权重, 使用 double) ---
		std::cout << "  Building Laplacian matrix L..." << std::endl;
		Eigen::SparseMatrix<double> L(n_vertices, n_vertices);
		std::vector<Eigen::Triplet<double>> L_triplets;
		L_triplets.reserve(n_vertices * 7); // 预估内存 (平均度数约为 6)

		if (use_cotangent_input) {
            // --- Cotangent Weights Calculation (Adapted from min_surf) ---
            for (auto v_it = mesh->vertices_begin(); v_it != mesh->vertices_end(); ++v_it)
            {
                // Get index 'i' for the current vertex v_it
                // int i = vertex_indices[*v_it]; // Use map if built
                 size_t i = v_it->idx();      // Or use direct index

                // For Laplacian Editing, ALL vertices contribute, even boundary ones
                // (Unlike Tutte/Minimal Surfaces where boundary rows are identity)

                double weight_sum = 0.0; // For diagonal L_ii

                // Iterate over outgoing halfedges from vertex i
                for (auto voh_it = mesh->voh_iter(*v_it); voh_it.is_valid(); ++voh_it)
                {
                    auto he_ij = *voh_it; // Halfedge from i to j
                    auto he_ji = mesh->opposite_halfedge_handle(he_ij); // Halfedge from j to i

                    auto v_j_h = mesh->to_vertex_handle(he_ij); // Neighbor vertex j handle
                    // int j = vertex_indices[v_j_h]; // Use map if built
                    size_t j = v_j_h.idx();         // Or use direct index

                    // Calculate cotangents of opposite angles using the helper function
                    double cot_alpha = 0.0, cot_beta = 0.0;

                    // Angle alpha is opposite edge ij in the face containing he_ij
                    if (!mesh->is_boundary(he_ij)) {
                        cot_alpha = OpenMeshUtils::cotangent_weight(*mesh, he_ij);
                    }

                    // Angle beta is opposite edge ij (or ji) in the face containing he_ji
                    if (mesh->is_valid_handle(he_ji) && !mesh->is_boundary(he_ji)) {
                        cot_beta = OpenMeshUtils::cotangent_weight(*mesh, he_ji);
                    }

                    // Standard cotangent weight formula
                    double weight = (cot_alpha + cot_beta) * 0.5;

                    // Clamp negative weights to zero (safer for editing, adjust if needed)
                    weight = std::max(0.0, weight);
                    // Optional: clamp large weights from degenerate triangles if helper returns large values
                    // weight = std::min(weight, 1e6); // Example clamp

                    // Add off-diagonal entry L_ij = -w_ij
                    L_triplets.emplace_back(i, j, -weight);

                    // Accumulate sum for diagonal entry L_ii = sum_j w_ij
                    weight_sum += weight;
                }

                // Add diagonal entry L_ii
                // Ensure diagonal is non-negative if weights were clamped
                // weight_sum = std::max(0.0, weight_sum);
                L_triplets.emplace_back(i, i, weight_sum);
            } // End vertex iteration for cotangent

        } else {
            // --- Uniform Weights Calculation (Original code) ---
            for (auto vh = mesh->vertices_begin(); vh != mesh->vertices_end(); vh++) {
                size_t i = vh->idx(); double degree = 0.0;
                for (auto vv_it = mesh->vv_iter(vh); vv_it.is_valid(); ++vv_it) {
                    size_t j = vv_it->idx();
                    if (i != j) {
                        L_triplets.emplace_back(i, j, -1.0);
                        degree += 1.0;
                    }
                }
                L_triplets.emplace_back(i, i, degree);
            }
        } // End if/else for weight type

		L.setFromTriplets(L_triplets.begin(), L_triplets.end());
        L.makeCompressed();

		// --- c) 计算并存储 Delta = L * V_orig ---
		//    Delta 是原始形状的 "拉普拉斯特征"
		std::cout << "  Calculating and storing Delta = L * V_orig..." << std::endl;
		storage.Delta = L * V_orig; // 存储在 storage 中以备后用
		// 检查计算结果是否有效
		if (!storage.Delta.allFinite())
		{
			std::cerr << "Mesh Editing Error: Calculated Delta contains NaN/Inf values. Check Laplacian matrix or input vertices." << std::endl;
			return false; // 计算出错，无法继续
		}

		// --- d) 构建组合矩阵 A = [L; w*C] (使用 double) ---
		//    这个矩阵用于求解系统，我们只在需要重新计算时构建它
		storage.constraint_weight = current_constraint_weight; // 更新/存储使用的权重
		std::cout << "  Building combined matrix A = [L; w*C] (constraint weight=" << storage.constraint_weight << ")..." << std::endl;
		Eigen::SparseMatrix<double> A(n_vertices + n_control_points, n_vertices);
		std::vector<Eigen::Triplet<double>> A_triplets;
		A_triplets.reserve(L.nonZeros() + n_control_points); // 预分配空间

		// 添加 L 矩阵部分到 A 的上部
		for (int k = 0; k < L.outerSize(); ++k)
		{
			for (Eigen::SparseMatrix<double>::InnerIterator it(L, k); it; ++it)
			{
				A_triplets.emplace_back(it.row(), it.col(), it.value());
			}
		}
		// 添加带权重的约束 C 部分到 A 的下部
		for (size_t i = 0; i < n_control_points; ++i)
		{
			size_t vertex_idx = control_points_indices[i];
			// A 的第 n_vertices + i 行，第 vertex_idx 列设为权重
			A_triplets.emplace_back(n_vertices + i, vertex_idx, storage.constraint_weight);
		}
		A.setFromTriplets(A_triplets.begin(), A_triplets.end());
		A.makeCompressed(); // 压缩

		// --- e) 执行 QR 分解 (compute) ---
		//    这是预计算中最耗时的步骤
		std::cout << "  Computing SparseQR decomposition of A..." << std::endl;
		if (!verify_matrix_is_finite(A))
		{ // 检查 A 是否有效
			std::cerr << "Mesh Editing Error: Matrix A contains invalid values before decomposition." << std::endl;
			return false;
		}
		storage.solver.compute(A); // <<< 核心预计算

		// --- f) 检查分解结果 ---
		if (storage.solver.info() != Eigen::Success)
		{
			// 分解失败，可能是矩阵 A 的问题 (例如奇异、数值不稳定等)
			std::cerr << "Mesh Editing Error: SparseQR decomposition failed! Eigen Error Code: " << storage.solver.info() << std::endl;
			std::cerr << "  Matrix A dimensions: " << A.rows() << "x" << A.cols() << ", Non-zeros: " << A.nonZeros() << std::endl;
			// 保持 solver_ready = false
			return false;
		}

		// --- g) 更新存储状态，标记预计算完成 ---
		storage.num_vertices = n_vertices;					 // 存储当前顶点数
		storage.control_indices_hash = current_indices_hash; // 存储当前控制点哈希
		storage.solver_ready = true;						 // <<< 标记求解器已就绪
		std::cout << "Solver precomputation finished successfully." << std::endl;
	}
	else
	{
		// 如果不需要重新计算，跳过上述步骤，直接使用存储的 solver 和 Delta
		std::cout << "Using precomputed solver and Delta." << std::endl;
	}

	// --- 7. 准备右侧向量 b = [Delta; w*P] ---
	//    这一步每次执行都需要，因为 P (控制点目标位置) 可能改变

	if (!storage.solver_ready)
	{
		// 防御性检查，如果前面的步骤失败，则不能继续
		std::cerr << "Mesh Editing Error: Solver is not ready (precomputation might have failed), cannot proceed." << std::endl;
		return false;
	}
	// 再次检查存储的 Delta 是否与当前顶点数匹配
	if (storage.Delta.rows() != n_vertices)
	{
		std::cerr << "Mesh Editing Error: Stored Delta size mismatch! Expected " << n_vertices << " rows, got " << storage.Delta.rows() << ". Forcing recompute might be needed." << std::endl;
		storage.solver_ready = false; // 标记状态不一致
		return false;
	}

	std::cout << "Building right-hand side vector b..." << std::endl;
	Eigen::MatrixXd b(n_vertices + n_control_points, 3); // b 是稠密矩阵 (N+M) x 3

	// --- a) 设置 b 的 Delta 部分 (从 storage 中获取) ---
	b.topRows(n_vertices) = storage.Delta;

	// --- b) 设置 b 的 P 部分 (控制点目标位置，乘以权重) ---
	for (size_t i = 0; i < n_control_points; ++i)
	{
		size_t original_idx = control_points_indices[i];

		// 检查索引是否在 changed_vertices 范围内
		if (original_idx >= changed_vertices.size())
		{
			std::cerr << "Mesh Editing Error: Control point index " << original_idx
					  << " is out of bounds for 'Changed vertices' array (size=" << changed_vertices.size()
					  << ") when building vector b." << std::endl;
			return false;
		}

		// 从 changed_vertices 获取目标位置 (pxr::GfVec3f)
		const auto &target_pos = changed_vertices[original_idx];

		// 填充 b 的对应行，转换为 double 并乘以权重
		b.row(n_vertices + i) = Eigen::Vector3d(
									static_cast<double>(target_pos[0]),
									static_cast<double>(target_pos[1]),
									static_cast<double>(target_pos[2])) *
								storage.constraint_weight; // 使用存储的权重
	}

	// 检查 b 是否包含无效值
	if (!b.allFinite())
	{
		std::cerr << "Mesh Editing Error: Right-hand side vector 'b' contains NaN/Inf values. Check Delta or control point positions." << std::endl;
		// 可以在这里打印 b 的内容帮助调试
		// std::cout << "DEBUG: Vector b:\n" << b << std::endl;
		return false;
	}

	// --- 8. 求解线性系统 Ax = b (使用预计算的 QR 分解) ---
	//    这一步利用了之前 compute() 的结果，主要是回代，速度较快
	std::cout << "Solving system Ax = b using precomputed SparseQR..." << std::endl;
	Eigen::MatrixXd X = storage.solver.solve(b); // <<< 快速求解步骤

	// 检查求解是否成功 (QR 的 solve 阶段通常比较稳定，但仍需检查)
	if (storage.solver.info() != Eigen::Success)
	{
		std::cerr << "Mesh Editing Error: Solver::solve() failed after decomposition! Eigen Error Code: " << storage.solver.info() << std::endl;
		// 可能意味着 b 向量有问题或分解后的数值问题
		return false;
	}

	// 检查解 X 是否包含无效值
	if (!X.allFinite())
	{
		std::cerr << "Mesh Editing Error: Solution matrix 'X' contains NaN/Inf values. Problem might be ill-conditioned." << std::endl;
		// std::cout << "DEBUG: Solution X:\n" << X << std::endl;
		return false;
	}

	// --- 9. 格式化并设置输出 ---
	//    将求解得到的 Eigen::MatrixXd 转换为 pxr::VtArray<pxr::GfVec3f>
	std::cout << "System solved successfully. Formatting output vertices..." << std::endl;
	// 打印解的范围有助于调试
	// std::cout << "  Solution range: min=" << X.minCoeff() << ", max=" << X.maxCoeff() << std::endl;

	pxr::VtArray<pxr::GfVec3f> new_positions(n_vertices); // 创建输出数组
	for (size_t i = 0; i < n_vertices; ++i)
	{
		// 从 double 解矩阵 X 获取新位置，并转换回 float (假设 GfVec3f 使用 float)
		new_positions[i] = pxr::GfVec3f(
			static_cast<float>(X(i, 0)),
			static_cast<float>(X(i, 1)),
			static_cast<float>(X(i, 2)));
	}

	// 设置节点的输出端口
	params.set_output("New vertices", std::move(new_positions));

	std::cout << "Mesh editing node execution finished successfully." << std::endl;
	return true; // 表示节点成功执行
}

// --- UI 声明 (保持不变，由框架提供) ---
NODE_DECLARATION_UI(mesh_editing);

NODE_DEF_CLOSE_SCOPE // GCore 框架宏
